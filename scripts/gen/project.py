__all__ = [
    'OS', 'INTRINSICS',
    'Project',
    'BaseModule', 'Module', 'Intrinsic', 'ExternalLibrary',
    'BundledLibrary', 'LibraryGroup', 'PkgConfig', 'Framework',
    'SdlConfig', 'LibrarySearch', 'TestSource', 'Executable',
    'Feature', 'Alternatives', 'Alternative', 'Require',
    'Variant', 'Defaults',
]
from gen.path import common_ancestor
from gen.error import ConfigError

# Intrinsics for each OS
OS = {
    'linux': ('linux', 'posix'),
    'osx': ('osx', 'posix'),
    'windows': ('windows',),
}
INTRINSICS = set(x for v in OS.values() for x in v)

########################################
# Project

class Project(object):
    """A project is a set of modules and project-wide settings."""

    __slots__ = [
        'name', 'ident', 'filename', 'url', 'email', 'copyright',
        'cvar', 'modules', 'module_names',
        'module_path', 'lib_path', 'defaults',
    ]

    def __init__(self):
        self.name = None
        self.ident = None
        self.filename = None
        self.url = None
        self.email = None
        self.copyright = None
        self.cvar = []
        self.modules = []
        self.module_names = {}
        self.module_path = []
        self.lib_path = []
        self.defaults = []

    def add_module(self, module):
        if module.modid is not None:
            if module.modid in self.module_names:
                raise ValueError('duplicate module name: {}'
                                 .format(module.modid))
            self.module_names[module.modid] = module
        self.modules.append(module)

    def targets(self):
        """Iterate over all target modules."""
        for m in self.modules:
            if m.is_target:
                yield m

    def trim(self):
        """Remove modules not reachable from any target.

        Return the set of module names that are referenced but do not
        exist.
        """
        q = list(self.targets())
        new_modules = list(q)
        visited = set()
        missing = set()
        mnames = {}
        while q:
            m = q.pop()
            cmodules = set()
            for c in m.configs():
                cmodules.update(c.modules)
            cmodules.difference_update(visited)
            for modid in cmodules:
                try:
                    m = self.module_names[modid]
                except KeyError:
                    missing.add(modid)
                    continue
                q.append(m)
                new_modules.append(m)
                mnames[modid] = m
            visited.update(cmodules)
        missing.difference_update(INTRINSICS)

        self.modules = new_modules
        self.module_names = mnames
        return missing

    def validate(self):
        flagids = set()
        dup_flagids = set()
        for m in self.modules:
            for c in m.configs():
                try:
                    flagid = c.flagid
                except AttributeError:
                    pass
                else:
                    if flagid in flagids:
                        dup_flagids.add(flagid)
                    else:
                        flagids.add(flagid)
        if dup_flagids:
            raise ConfigError(
                'duplicate flags: {}'
                .format(', '.join(sorted(dup_flagids))))

    def configs(self, enable=None):
        """Iterate over all config objects in the project."""
        for m in self.modules:
            for c in m.configs(enable):
                yield c

    def closure(self, modules):
        """Get a list of all modules the given modules depend on.

        This returns (mods, unsat), where mods is a list of the
        modules (including the original modules), and unsat is a list
        of tags which are unsatisfied.
        """
        used = set()
        q = list(modules)
        unsat = []
        mods = list(q)
        while q:
            m = q.pop()
            deps = set(m.require)
            for f in m.feature:
                for i in f.impl:
                    deps.update(i.require)
            for v in m.variant:
                deps.update(v.require)
            for mid in deps.difference(used):
                try:
                    m = self.module_names[mid]
                except KeyError:
                    unsat.append(mid)
                else:
                    mods.append(m)
                    q.append(m)
            used.update(deps)
        return mods, unsat

    def features(self, modules=None):
        """Get a list of features for the given modules.

        If no iterable of modules is supplied, then all features are
        returned.
        """
        if modules is not None:
            mods, unsat = self.closure(modules)
        else:
            mods = self.modules
        a = []
        for m in mods:
            a.extend(m.feature)
        return a

    def variants(self, targets=None):
        """Get a list of variants for the given modules.

        If no iterable of modules is supplied, then all variants are
        returned.
        """
        if modules is not None:
            mods, unsat = self.closure(modules)
        else:
            mods = self.modules
        a = []
        for m in mods:
            a.extend(m.variant)
        return a

    def sources(self, target, getenv):
        """Iterate over the sources in a given target.

        Yields (source, env) pairs, where env is the environment for
        building the given source.  The environment is taken from the
        getenv function, which should map a module name and a set of
        tags to either the corresponding environment if the source
        should be built, or None if the source should not be built.
        """
        modules = set()
        mlist = [target]

        while mlist:
            m = mlist.pop()

########################################
# Modules

class BaseModule(object):
    """A module.

    A project module is a group of sources.  Modules can depend on
    other modules, so when one module is built, dependent modules will
    also be built and linked in.  All paths are relative to the module
    root, and the module root is a native path.  Concrete module
    subclasses can be just source code, or they can be targets such as
    executables.

    modid: identifies the module

    name: human-readable description

    header_path: list of header include paths, both for this module
    and any source depending on it, including indirect dependencies

    define: list of preprocessor definitions, both for this module and
    any source depending on it, including indirect dependencies

    require: list of modules this module depends on

    cvar: list of cvars and values to be set by default for developers

    sources: list of source files

    feature: list of features this module has
    """

    __slots__ = [
        'modid', 'name',
        'header_path', 'define', 'cvar',
        'sources', 'config', 'defaults',
    ]

    def __init__(self, modid):
        self.modid = modid
        self.name = None
        self.header_path = []
        self.define = []
        self.cvar = []
        self.sources = []
        self.config = []
        self.defaults = []

    def __repr__(self):
        if self.modid is not None:
            return '<{} {}>'.format(self.__class__.__name__, self.modid)
        else:
            return '<{} {:#}>'.format(self.__class__.__name__, id(self))

    @property
    def is_target(self):
        """Whether this module is a buildable target."""
        return False

    def add_source(self, src):
        self.sources.append(src)

    def module_root(self):
        """Get the directory containing all module sources.

        This will throw an exception for modules containing no sources
        (e.g., external libraries).
        """
        # FIXME: use of this method is sign of a HACK
        return common_ancestor(src.path for src in self.sources)

    def configs(self, enable=None):
        """Iterate over all configs recursively."""
        for c in self.config:
            for cc in c.configs(enable):
                yield cc

class Module(BaseModule):
    """Simple module with a fixed location and sources."""

    __slots__ = BaseModule.__slots__

class Intrinsic(BaseModule):
    """Module which is intrinsically present or not present.

    There is no XML representation for this kind of module.
    """

    __slots__ = BaseModule.__slots__

    def __repr__(self):
        return '<Intrinsic {}>'.format(self.modid)

class ExternalLibrary(BaseModule):
    """Module which is an external library.

    A module has a number possible library sources.  Each library
    source specifies a method of locating the library, finding its
    header search paths, and linking the library into a program.

    When the project is configured or reconfigured, the library
    directory will be searched for bundled libraries.  Any library
    which is found will be used to fill in the base module properties:
    header_path, sources, et cetera, and the use_bundled attribute
    will be set to True.
    """

    __slots__ = (BaseModule.__slots__ +
                 ['libsources', 'bundled_versions', 'use_bundled'])

    def __init__(self, modid):
        super(ExternalLibrary, self).__init__(modid)
        self.libsources = []
        self.bundled_versions = []
        self.use_bundled = False

    def __repr__(self):
        return '<ExternalLibrary {}>'.format(self.modid)

    def add_libsource(self, source):
        self.libsources.append(source)

class BundledLibrary(object):
    """Library source for libraries bundled with the project.

    The 'lib' directory will be searched for a directory matching the
    specified 'libname', and that library will be used as the root
    path.  A bundled library is otherwise similar to an ordinary
    module.
    """

    __slots__ = ['libname', 'header_path', 'define',
                 'cvar', 'config', 'sources']

    def __init__(self, libname):
        self.libname = libname
        self.header_path = []
        self.define = []
        self.cvar = []
        self.config = []
        self.sources = []

    def add_source(self, source):
        self.sources.append(source)

class LibraryGroup(object):
    """Library source which aggregates sources.

    All library sources in the group must be present.  Bundled
    libraries can not be included inside the group.
    """

    __slots__ = ['libsources']

    def __init__(self):
        self.libsources = []

    def add_libsource(self, source):
        self.libsources.append(source)

class PkgConfig(object):
    """Library source which uses pkg-config to find a library."""

    __slots__ = ['spec']

    def __init__(self, spec):
        self.spec = spec

    srcname = 'pkgconfig'

class Framework(object):
    """Library source which is a Darwin framework."""

    __slots__ = ['name']

    def __init__(self, name):
        self.name = name

    srcname = 'framework'

class SdlConfig(object):
    """Library source which uses sdl-config to find LibSDL."""

    __slots__ = ['version']

    def __init__(self, version):
        self.version = version

    srcname = 'sdlconfig'

class LibrarySearch(object):
    """Library source which searches for a library.

    This creates a source file, and then tries a series of different
    compilation flags until it finds a set that can compile and link
    the source file.
    """

    __slots__ = ['source', 'flags']

    def __init__(self, source, flags):
        self.source = source
        self.flags = flags

    srcname = 'librarysearch'

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

class Executable(Module):
    """Module that produces an executable."""

    __slots__ = (BaseModule.__slots__ +
                 ['exe_name', 'exe_icon', 'apple_category'])

    @property
    def is_target(self):
        return True

    def __init__(self, modid):
        super(Executable, self).__init__(modid)
        self.exe_name = {}
        self.exe_icon = {}
        self.apple_category = None

########################################
# Config

class BaseConfig(object):
    def configs(self, enable=None):
        """Iterate over this and all child config objects."""
        assert False # Must be overridden by subclass

    modules = ()

class Feature(BaseConfig):
    """A feature is a part of the code that can be enabled or disabled."""

    __slots__ = ['flagid', 'name', 'config']

    def __init__(self, flagid):
        self.flagid = flagid
        self.name = None
        self.config = []

    def configs(self, enable=None):
        """Iterate over this and all child config objects."""
        if enable is None or self.flagid in enable:
            yield self
            for c in self.config:
                for x in c.configs(enable):
                    yield x

class Alternatives(BaseConfig):
    """Alternatives specify multiple ways to satisfy requirements."""

    __slots__ = ['alternatives']

    def __init__(self):
        self.alternatives = []

    def configs(self, enable=None):
        """Iterate over this and all child config objects."""
        yield self
        for c in self.alternatives:
            for x in c.configs(enable):
                yield x

class Alternative(BaseConfig):
    __slots__ = ['flagid', 'name', 'config']

    def __init__(self, flagid):
        self.flagid = flagid
        self.name = None
        self.config = []

    def configs(self, enable=None):
        """Iterate over this and all child config objects."""
        if enable is None or self.flagid in enable:
            yield self
            for c in self.config:
                for x in c.configs(enable):
                    yield x

class Require(BaseConfig):
    __slots__ = ['modules']

    def __init__(self, modules):
        self.modules = tuple(modules)

    def configs(self, enable=None):
        """Iterate over this and all child config objects."""
        yield self

class Variant(BaseConfig):
    """A variant is a different version of the code that can be built.

    Multiple variants can be built in parallel, but only one will ever
    be linked into the same executable.
    """

    __slots__ = ['flagid', 'name', 'config', 'exe_suffix']

    def __init__(self, flagid):
        self.flagid = flagid
        self.name = None
        self.config = []
        self.exe_suffix = {}

    def configs(self, enable=None):
        """Iterate over this and all child config objects."""
        if enable is None or self.flagid in enable:
            yield self
            for c in self.config:
                for x in c.configs(enable):
                    yield x

########################################
# Misc

class Defaults(object):
    """A defaults object controls the default libraries to use."""

    __slots__ = ['os', 'enable']

    def __init__(self, os, enable):
        self.os = os
        self.enable = tuple(enable)
