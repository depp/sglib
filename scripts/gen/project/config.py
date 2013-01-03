"""Project configuration objects.

A configuration object represents configuration in a very general
sense.  Configuration objects can pull in dependent modules, set paths
to search for header files, et cetera.  Configuration objects can also
be nested to create groups of settings which are enabled or disabled
during project configuration.
"""
__all__ = [
    'ConfigSet', 'BaseConfig',
    'Feature', 'Alternatives', 'Alternative', 'Require',
    'Public', 'Variant', 'Defaults'
]
import argparse
import io
from gen.error import ConfigError
from gen.path import Path

########################################################################
# Abstract base class for all config objects

class BaseConfig(object):
    """Abstract base class for configuration objects.

    Config objects optionally have the following attributes.  If the
    config object does not support the attribute, AttributeError will
    be raised as normal, but all config objects which have the
    attribute will use them in the same way.

    modules: a list of modules which are required

    flagid: the enable flag for this config object
    """

    def children(self):
        """Iterate over the direct children of this config object."""
        return ()

    def iter_configs(self):
        """Iterate over all config objects.

        Yields pairs (config,public), indicating which config objects
        are public and which are private.
        """
        q = [(self, False)]
        while q:
            c, public = q.pop()
            yield c, public
            public = public or isinstance(c, Public)
            q.extend((cc, public) for cc in c.children())

    def all_configs(self):
        """Iterate over all config objects."""
        return (c for c, public in self.iter_configs())

    def public_configs(self):
        """Iterate over all public config objects."""
        return (c for c, public in self.iter_configs() if public)

    def private_configs(self):
        """Iterate over all private config objects."""
        return (c for c, public in self.iter_configs() if not public)

    def prefix_paths(self, prefix):
        """Modify the config by prefixing all paths with a prefix."""
        for c in self.all_configs():
            c._prefix_paths(prefix)

    def _prefix_paths(self, prefix):
        pass

    def module_deps(self):
        """Return a set of all module references."""
        mod_deps = set()
        for c in self.all_configs():
            try:
                cmodules = c.modules
            except AttributeError:
                pass
            else:
                mod_deps.update(cmodules)
        return mod_deps

    def enable_flags(self):
        """Return a set of all enable flags this config has."""
        flags = set()
        for c in self.all_configs():
            try:
                flagid = c.flagid
            except AttributeError:
                pass
            else:
                flags.add(flagid)
        return flags

    def add_options(self, parser):
        """Add config options to the given ArgumentParser."""

        groups = {}
        gfeat = parser.add_argument_group('optional features')
        galt = parser.add_argument_group('optional packages')
        gvar = parser.add_argument_group('build variants')

        for c in self.all_configs():
            try:
                flagid = c.flagid
            except AttributeError:
                continue
            enable_flag, disable_flag = c.flag_names()
            enable_help = c.flag_help()
            gname, gdesc = c.flag_groupname()
            try:
                group = groups[gname]
            except KeyError:
                group = parser.add_argument_group(gname, gdesc)
                groups[gname] = group
            varname = 'enable:' + flagid
            group.add_argument(
                enable_flag,
                default=None,
                action='store_true',
                dest=varname,
                help=enable_help)
            group.add_argument(
                disable_flag,
                default=None,
                action='store_false',
                dest=varname,
                help=argparse.SUPPRESS)

    def get_enables(self, opts, os):
        """Get the enable and variant flags from the given options.

        Returns (enables,variants).
        """
        errors = []

        # Get the set of default and user flags
        # Also get map of conflicting flags
        denabled = set() # Defaults for this platform
        uenabled = set() # User enabled flags
        udisabled = set() # User disabled flags
        conflicts = {} # Map of conflicting flags
        variants = set()
        for c in self.all_configs():
            if isinstance(c, Defaults):
                if d.os is None or os == d.os:
                    denabled.update(d.enable)
            try:
                flagid = c.flagid
            except AttributeError:
                pass
            else:
                val = getattr(opts, 'enable:' + flagid)
                if val is not None:
                    (uenabled if val else udisabled).add(flagid)
            if isinstance(c, Alternatives):
                aflags = set(cc.flagid for cc in c.children())
                for aflag in aflags:
                    try:
                        conflicts[aflag] = aflags.union(conflicts[aflag])
                    except KeyError:
                        conflicts[aflag] = aflags
            elif isinstance(c, Variant):
                variants.add(flagid)

        def describe_conflicts(cflags, desc):
            fp = io.StringIO()
            fp.write('error: {}\n'.format(desc))
            flagsets = set()
            flagnames = {}
            for c in self.all_configs():
                try:
                    flagid = c.flagid
                except AttributeError:
                    continue
                flagnames[flagid] = c.flag_name()[0]
            for flagid in econflict:
                flags = conflicts[flagid].intersection(econflict)
                assert len(flags) >= 2
                flags = tuple(sorted(flags))
                if flags in flagsets:
                    continue
                flagsets.add(flags)
                fp.write('flags conflict: {}\n'
                         .format(' '.join(
                             flagnames[flagid] for flagid in flags)))
            return fp.getvalue()

        # Extend the definition of "user disabled flags" to include
        # flags that conflict with user enabled flags
        for flagid in uenabled:
            try:
                c = conflicts[flagid]
            except KeyError:
                continue
            udisabled.update(c.difference([flagid]))

        # Find conflicting user flags
        econflict = udisabled.intersection(uenabled)
        if econflict is not None:
            errors.append(describe_conflicts(
                econflict, 'conflicting flags specified'))

        # Find conflicting default flags
        defaults = denabled.difference(udisabled)
        dconflict = set()
        for flagid in defaults:
            try:
                c = conflicts[flagid]
            except KeyError:
                continue
            dconflict.update(c)
        if dconflict:
            errors.append(describe_conflicts(
                econflict, 'default flags are in conflict'))

        enabled = uenabled.union(defaults)

        # Find unsatisfied alternatives
        q = [((), self)]
        while q:
            f, c = q.pop()
            try:
                flagid = c.flagid
            except AttributeError:
                pass
            else:
                if flagid not in enabled:
                    continue
                flagname = c.flag_names()[0]
                if flagid in uenabled:
                    flagname = '{} (default)'.format(flagname)
                f = f + (flagname,)
            count = 0
            if isinstance(c, Alternatives):
                for cc in c.children():
                    count += cc.flagid in enabled
            if count > 1:
                assert errors
            if not count:
                fp = io.StringIO()
                fp.write('error: requirement unsatisfied\n')
                if f:
                    fp.write(
                        'requirement is enabled by flags: {}\n'
                        .format(', '.join(f)))
                else:
                    fp.write('requirement is mandatory\n')
                fp.write(
                    'requirement may be satisfied by specifying one '
                    'of the following flags:\n')
                fp.write('    {}\n'
                         .format(', '.join(cc.flag_names()[0]
                                           for cc in c.children())))
                errors.append(fp.getvalue())

        if errors:
            raise ConfigError('invalid configuration', '\n'.join(errors))

        return (enabled.difference(variants),
                enabled.intersection(variants))

    def apply_config(self, config):
        """Apply configuration.

        Returns (mods,pub,priv), where 'mods' is the list of public
        module dependencies, 'pub' is the public environment, and
        'priv' is the private environment.
        """
        modules = set()
        env_public = []
        env_private = []
        q = [(False, self)]
        while q:
            public, c = q.pop()
            public = public or isinstance(c, Public)
            try:
                flagid = c.flagid
            except AttributeError:
                pass
            else:
                if flagid not in config.enable:
                    continue
            if public:
                try:
                    cmodules = c.modules
                except AttributeError:
                    pass
                else:
                    modules.update(cmodules)
            env = c.create_env(config)
            if not isinstance(env, dict):
                raise TypeError('create_env should return a dict')
            if env:
                (env_public if public else env_private).append(env)
            q.extend((public, cc) for cc in c.children())
        return (modules,
                env.merge_env(env_public),
                env.merge_env(env_private))

    def variant_config(self, enable, variant):
        """Get info for a given variant.

        Returns (mods,cvars) where 'mods' is a list of dependent
        modules for that variant, and cvars is a list of CVars for
        that variant.
        """
        q = [self]
        flagids = set(enable)
        flagids.add(variant)
        modules = set()
        cvars = []
        while q:
            c = q.pop()
            try:
                flagid = c.flagid
            except AttributeError:
                pass
            else:
                if flagid not in flagids:
                    continue
            try:
                cmodules = c.modules
            except AttributeError:
                pass
            else:
                modules.update(cmodules)
            if isinstance(c, CVar):
                cvars.append((c.name, c.value))
        return modules, cvars

    def create_env(self, config):
        return {}

    def validate(self, enable, is_variant=False):
        for c in self.children():
            c.validate(enable, is_variant)

########################################################################
# Configuration groups

class ConfigSet(BaseConfig):
    """A configuration object containing other configuration objects."""

    __slots__ = ['_children']

    def __init__(self):
        super(ConfigSet, self).__init__()
        self._children = []

    def add_config(self, config):
        """Add a configuration object."""
        self._children.append(config)

    def children(self):
        """Iterate over the direct children of this config object."""
        return iter(self._children)

    def apply_enables_iter(self, enabled):
        """Iterate over enabled configuration objects."""
        try:
            flagid = self.flagid
        except KeyError:
            pass
        else:
            if flagid not in enabled:
                return
        for c in self._children:
            for cc in c.apply_enables_iter(enabled):
                yield cc

    def create_env(self, config):
        return {}

class Feature(ConfigSet):
    """A feature is a part of the code that can be enabled or disabled.

    It consists of a flagid, which is used to enable or disable the
    object, and a collection of child configuration objects, which are
    included in target configuration if the flagid is enabled.
    """

    __slots__ = ['flagid', 'name', '_children']

    def __init__(self, flagid):
        super(Feature, self).__init__()
        self.flagid = flagid
        self.name = None

    def flag_names(self):
        return ('--enable-{}'.format(self.flagid), 
                '--disable-{}'.format(self.flagid))

    def flag_groupname(self):
        return 'optional features', None

    def flag_help(self):
        return 'enable {}'.format(self.name)

class Alternatives(ConfigSet):
    """Alternatives specify multiple ways to satisfy requirements.

    It consists of several alternatives, exactly one of which should
    be enabled.
    """

    __slots__ = ['_children']

    def add_config(self, config):
        if not isinstance(config, Alternative):
            raise TypeError('children of Alternatives must be Alternative')
        super(Alternatives, self).add_config(config)

class Alternative(ConfigSet):
    """An individual alternative in a group of alternatives."""

    __slots__ = ['flagid', 'name', '_children']

    def __init__(self, flagid):
        super(Alternative, self).__init__()
        self.flagid = flagid
        self.name = None

    def flag_names(self):
        return ('--with-{}'.format(self.flagid), 
                '--without-{}'.format(self.flagid))

    def flag_groupname(self):
        return 'optional packages', None

    def flag_help(self):
        return 'use {}'.format(self.name)

class Public(ConfigSet):
    """A public config makes all of its children public."""

    __slots__ = ['_children']

class Variant(ConfigSet):
    """A variant is a different version of the code that can be built.

    Multiple variants can be built in parallel, but only one will ever
    be linked into the same executable.
    """

    __slots__ = ['flagid', 'name', 'filename', '_children']

    def __init__(self, flagid):
        super(Variant, self).__init__()
        self.flagid = flagid
        self.name = None
        self.filename = {}

    def flag_names(self):
        return ('--enable-{}'.format(self.flagid), 
                '--disable-{}'.format(self.flagid))

    def flag_groupname(self):
        return 'build variants', None

    def flag_help(self):
        return 'build {}'.format(self.name)

########################################################################
# Simple config objects

class Require(BaseConfig):
    """A configuration which requires certain modules or flags.

    If the requirements are global, then they apply to every source in
    the module.
    """

    __slots__ = ['modules', 'enable', 'is_global']

    def __init__(self, modules, enable, is_global):
        self.modules = tuple(modules)
        self.enable = tuple(enable)
        self.is_global = bool(is_global)

class Defaults(BaseConfig):
    """A defaults object specifies default enable flags."""

    __slots__ = ['os', 'enable']

    def __init__(self, os, enable):
        self.os = os
        self.enable = tuple(enable)

class CVar(BaseConfig):
    """A configuration which sets a CVar."""

    __slots__ = ['name', 'value']

    def __init__(self, name, value):
        self.name = name
        self.value = tuple(value)

    def _prefix_paths(self, prefix):
        value = []
        for v in self.value:
            if isinstance(v, Path):
                v = Path(prefix, v)
            value.append(v)
        self.value = tuple(value)

class HeaderPath(BaseConfig):
    """A header search path."""

    __slots__ = ['path']

    def __init__(self, path):
        self.path = path

    def create_env(self, config):
        return { 'CPPPATH': (self.path,) }

    def _prefix_paths(self, prefix):
        self.path = Path(prefix, self.path)

class Define(BaseConfig):
    """A configuration which defines a preprocessor macro."""

    __slots__ = ['name', 'value']

    def __init__(self, name, value):
        self.name = name
        self.value = value

    def create_env(self, config):
        return { 'DEFS': ((self.name, self.value),) }

########################################################################
# Library source config objects

class PkgConfig(BaseConfig):
    """Use pkg-config to find a library."""

    __slots__ = ['spec']

    def __init__(self, spec):
        self.spec = spec

    def create_env(self, config):
        return config.config_pkgconfig(config, self.spec)

class Framework(BaseConfig):
    """Link with a Darwin framework."""

    __slots__ = ['name']

    def __init__(self, name):
        self.name = name

    def create_env(self, config):
        return config.config_framework(config, self.name)

class SdlConfig(BaseConfig):
    """Use sdl-config to find LibSDL."""

    __slots__ = ['version']

    def __init__(self, version):
        self.version = version

    def create_env(self, config):
        return config.config_sdlconfig(config, self.version)

class Search(BaseConfig):
    """Search for a library.

    This creates a source file, and then tries a series of different
    compilation flags until it finds a set that can compile and link
    the source file.
    """

    __slots__ = ['source', 'flags']

    def __init__(self, source, flags):
        self.source = source
        self.flags = flags

    def create_env(self, config):
        return config.config_framework(config, self.source, self.flags)

class TestSource(object):
    """Test source file.

    Consists of a prologue and a body.  The prologue is placed at the
    top of the source file.  Below the prologue the main() function is
    defined, which contains the body followed by a return statement.
    """

    __slots__ = ['prologue', 'body']

    def __init__(self, prologue, body):
        self.prologue = prologue
        self.body = body

