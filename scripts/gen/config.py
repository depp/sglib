from gen.path import Path
from gen.source import Source
from gen.error import ConfigError
import gen.git as git
import optparse
import gen.project as project
import os
import sys
import platform

CACHE_FILE = 'config.dat'

# TODO: After loading the project,
# make sure each tag refers to one module

DEFAULT_ACTIONS = {
    'LINUX': ('makefile', 'runner', 'version'),
    'OSX': ('version'),
    'WINDOWS': ('version'),
}

def use_bundled_lib(config, lib, lsrc, fname):
    root = Path(config.project.lib_path, fname)
    if not config.quiet:
        sys.stderr.write('found bundled library: {}\n'.format(root.posix))

    lib.header_path = lsrc.header_path
    lib.define = lsrc.define
    lib.require = lsrc.require
    lib.cvar = lsrc.cvar
    lib.sources = [Source(Path(root, s.path), s.tags) for s in lsrc.sources]
    lib.use_bundled = True

def find_bundled_lib(config, lib, libs):
    for lsrc in lib.bundled_versions:
        searchname = lsrc.libname
        for fname in libs:
            i = fname.rfind('-')
            if i >= 0:
                libname = fname[:i]
            else:
                libname = fname
            if searchname == libname:
                use_bundled_lib(config, lib, lsrc, fname)
                return

def find_bundled_libs(config):
    """Scan the bundled library folder for bundled libraries.

    This will fill in the module fields for every ExternalLibrary that
    can be bundled, if that library is found in the library folder.
    """
    proj = config.project

    if proj.lib_path is None:
        return

    try:
        fnames = os.listdir(proj.lib_path.native)
    except OSError:
        return

    libs = []
    for fname in fnames:
        npath = os.path.join(proj.lib_path.native, fname)
        if fname.startswith('.'):
            continue
        if not os.path.isdir(npath):
            continue
        try:
            Path(fname)
        except ValueError:
            sys.stderr.write('warning: ignoring {}\n'.format(npath))
            continue
        libs.append(fname)

    for m in proj.modules:
        if (isinstance(m, project.ExternalLibrary) and
            m.bundled_versions and
            config.opts.get('bundled_' + m.modid, True)):
            find_bundled_lib(config, m, libs)

def trim_project(proj):
    """Remove unreferenced modules from the project."""
    mods, unsat = proj.closure(proj.targets())
    all_mods = set(proj.module_names)
    used_mods = set(mod.modid for mod in mods if mod.modid is not None)
    unused_mods = all_mods.difference(used_mods)
    for mid in unused_mods:
        del proj.module_names[mid]

def check_deps(proj):
    """Check for unsatisfied dependencies in the project."""
    mods, unsat = proj.closure(proj.targets())
    if unsat:
        sys.stderr.write(
            'warning: unknown module dependencies: {}\n'
            .format(', '.join(sorted(unsat))))
    all_tags = set()
    used_tags = set()
    for m in proj.modules:
        if m.modid is not None:
            all_tags.add(m.modid)
        all_tags.update(f.modid for f in m.feature)
        for s in m.sources:
            used_tags.update(s.tags)
    unsat_tags = used_tags.difference(all_tags)
    if unsat_tags:
        sys.stderr.write(
            'warning: unknown source tags: {}\n'
            .format(', '.join(sorted(unsat_tags))))

def parse_enable(keys, p, name, dest=None, default=None, help_enable=None,
                 help_disable=None):
    assert dest is not None
    keys.append(dest)
    if help_enable is None:
        help_enable = optparse.SUPPRESS_HELP
    if help_disable is None:
        help_disable = optparse.SUPPRESS_HELP
    p.add_option('--enable-' + name, default=default, action='store_true',
                 dest=dest, help=help_enable)
    p.add_option('--disable-' + name, default=default, action='store_false',
                 dest=dest, help=help_disable)

def parse_feature_args(proj, keys, p):
    gv = optparse.OptionGroup(p, 'Variants')
    ge = optparse.OptionGroup(p, 'Optional Features')
    gw = optparse.OptionGroup(p, 'Optional Packages')
    p.add_option_group(gv)
    p.add_option_group(ge)
    p.add_option_group(gw)

    for m in proj.modules:
        for v in m.variant:
            parse_enable(
                keys, gv, v.varname,
                dest='variant_' + v.varname,
                help_enable='build {}'.format(v.name))

    optlibs = set()
    for f in proj.features():
        if f.desc is not None:
            help_enable = 'enable {}'.format(f.desc)
        else:
            help_enable = 'enable {} feature'.format(f.modid)
        parse_enable(
            keys, ge, f.modid.lower(),
            dest='enable_' + f.modid,
            default=True,
            help_enable=help_enable)

        for i in f.impl:
            optlibs.update(i.require)

    del f

    for m in proj.modules:
        if not isinstance(m, project.ExternalLibrary):
            continue
        nm = m.modid.lower()
        desc = m.name if m.name is not None else '{} library'.format(modid)
        opts = []
        if m.modid in optlibs:
            opts.append((nm, 'with_' + m.modid, desc))
        if m.bundled_versions:
            opts.append(('bundled-' + nm, 'bundled_' + m.modid,
                         '{} (bundled copy)'.format(desc)))

        for argn, var, desc in opts:
            keys.append(var)
            gw.add_option(
                '--with-' + argn,
                default=None,
                action='store_true',
                dest=var,
                help='use {}'.format(desc))
            gw.add_option(
                '--without-' + argn,
                default=None,
                action='store_false',
                dest=var,
                help=optparse.SUPPRESS_HELP)

def parse_general(keys, p):
    p.add_option(
        '--debug',
        dest='config', default='release',
        action='store_const', const='debug',
        help='use debug configuration')
    p.add_option(
        '--release',
        dest='config', default='release',
        action='store_const', const='release',
        help='use release configuration')
    keys.append('config')

    parse_enable(
        keys, p, 'warnings',
        dest='warnings',
        help_enable='enable extra compiler warnings')
    parse_enable(
        keys, p, 'werror',
        dest='werror',
        help_enable='treat warnings as errors')

class ProjectConfig(object):
    """Project-wide configuration."""

    __slots__ = [
        'argv', 'xmlfiles', 'project', 'opts', 'vars',
        '_repos', '_versions', '_native_os', '_actions',
        '_executed', '_quiet',
    ]

    def __init__(self):
        self.argv = None
        self.xmlfiles = None
        self.project = None
        self.opts = None
        self.vars = None
        self._repos = None
        self._versions = None
        self._native_os = None
        self._actions = None
        self._executed = None
        self._quiet = None

    def __getstate__(self):
        d = dict()
        for attr in self.__slots__:
            if attr.startswith('_'):
                continue
            d[attr] = getattr(self, attr)
        return d

    def __setstate__(self, d):
        for attr in self.__slots__:
            if attr.startswith('_'):
                v = None
            else:
                v = d[attr]
            setattr(self, attr, v)

    def reconfig(self):
        import gen.xml as xml

        xmlfiles = [Path('project.xml')]
        proj = xml.load('project.xml', Path())
        for modpath in proj.module_path:
            for fname in os.listdir(modpath.native):
                if fname.startswith('.') or not fname.endswith('.xml'):
                    continue
                path = Path(modpath, fname)
                mod = xml.load(path.native, modpath)
                proj.add_module(mod)
                xmlfiles.append(path)
        self.xmlfiles = xmlfiles
        self.project = proj

        trim_project(proj)
        for tag in project.INTRINSICS:
            proj.add_module(project.Intrinsic(tag))
        check_deps(proj)

        p = optparse.OptionParser()
        keys = []
        parse_feature_args(proj, keys, p)
        parse_general(keys, p)

        opts, args = p.parse_args(self.argv)
        opt_dict = {}
        for k in keys:
            v = getattr(opts, k)
            if v is not None:
                opt_dict[k] = v

        var_dict = {}
        actions = []
        for arg in args:
            i = arg.find('=')
            if i >= 0:
                var_dict[arg[:i]] = arg[i+1:]
            else:
                actions.append(arg)

        self.opts = opt_dict
        self.vars = var_dict
        if not actions:
            actions = DEFAULT_ACTIONS[self.native_os]

        find_bundled_libs(self)

        return actions

    def is_enabled(self, featid):
        """Return the --enable state of the feature."""
        return self.opts['enable_' + featid]

    def is_with(self, modid):
        """Return the --with state of the library."""
        return self.opts.get('with_' + modid, None)

    def get_config(self, os):
        """Get the build configuration for the given os.

        Returns a BuildConfig object.
        """

        intrinsics = set(project.OS[os])

        dlist = self.project.defaults
        for m in self.project.modules:
            dlist.extend(m.defaults)

        # Get defaults
        enabled_variants = set()
        enabled_features = set()
        enabled_libs = set()
        for d in dlist:
            if d.os is not None and os not in d.os:
                continue
            if d.variants is not None:
                enabled_variants.update(d.variants)
            if d.features is not None:
                enabled_features.update(d.features)
            if d.libs is not None:
                enabled_libs.update(d.libs)

        # Override defaults with user settings
        features = list(self.project.features())
        for f in features:
            if self.is_enabled(f.modid):
                enabled_features.add(f.modid)
            else:
                enabled_features.discard(f.modid)
        user_libs = set()
        for modid in self.project.module_names:
            v = self.is_with(modid)
            if v is None:
                continue
            if v:
                enabled_libs.add(modid)
                user_libs.add(modid)
            else:
                enabled_libs.discard(modid)
        for m in self.project.modules:
            for v in m.variant:
                val = self.opts.get('variant_' + v.varname, None)
                if val is not None:
                    if val:
                        enabled_variants.add(v.varname)
                    else:
                        enabled_variants.discard(v.varname)

        # Add platform intrinsics (you can't disable WINDOWS on Windows)
        enabled_libs.update(intrinsics)

        # Remove defaults that conflict with user preferences
        # So you can specify --with-x without needing --without-y
        for f in features:
            if f.modid not in enabled_features:
                continue
            has_user = False
            for i in f.impl:
                if user_libs.intersection(i.require):
                    has_user = True
            if not has_user:
                continue
            for i in f.impl:
                if not user_libs.intersection(i.require):
                    enabled_libs.difference_update(i.require)

        # Check for conflicts and unsatisfied dependencies
        needed_libs = set()
        for f in features:
            if f.modid not in enabled_features:
                continue
            impls = []
            for i in f.impl:
                if all(lib in enabled_libs for lib in i.require):
                    impls.append(i)
            if not impls:
                raise ConfigError(
                    'unsatisfied feature: {}'.format(f.modid.lower()),
                    'libraries required for this feature are disabled\n'
                    'use --disable-{} if you want to disable this feature\n'
                    .format(f.modid.lower()))
            if len(impls) > 1:
                raise ConfigError(
                    'overly satisfied feature: {}'.format(f.modid.lower()),
                    'this feature is satisfied in multiple ways: {}\n'
                    'use fewer --with-lib options\n'
                    .format(', '.join(' '.join(i.require) for i in impls)))
            needed_libs.update(impls[0].require)
        needed_libs.difference_update(intrinsics)

        cfg = BuildConfig()
        cfg.project = self.project
        cfg.projectconfig = self
        cfg.os = os
        cfg.variants = frozenset(enabled_variants)
        cfg.features = frozenset(enabled_features)
        cfg.libs = frozenset(needed_libs)
        cfg._calculate_targets()
        return cfg

    @property
    def repos(self):
        if self._repos is None:
            if self.project is None:
                raise AttributeError('repos')
            self._repos = {
                'APP': Path(),
                'SG': self.project.module_names['SG'].module_root(),
            }
        return self._repos

    @property
    def versions(self):
        if self._versions is None:
            if self.project is None:
                raise AttributeError('repos')
            versions = {}
            for k, v in self.repos.items():
                versions[k] = git.get_info(self, v)
            self._versions = versions
        return self._versions

    @property
    def actions(self):
        if self._actions is None:
            from gen.build import version
            self._actions = {
                'makefile': (Path('Makefile'), 'linux.gen_makefile'),
                'version': (
                    Path(self.repos['SG'], 'sg/core/version_const.c'),
                    'version.gen_version'),
                'runner': (Path('run.sh'), 'runner.gen_runner'),
            }
        return self._actions

    @property
    def native_os(self):
        os = self._native_os
        if os is None:
            d = {
                'Darwin': 'OSX',
                'Linux': 'LINUX',
                'Windows': 'WINDOWS',
            }
            s = platform.system()
            try:
                os = d[s]
            except KeyError:
                raise ConfigError('unsupported platform: {}'.format(s))
            self._native_os = os
        return os

    def exec_action(self, action_name):
        if self._executed is None:
            self._executed = set()
        if action_name in self._executed:
            return
        actions = self.actions
        try:
            action = actions[action_name]
        except KeyError:
            raise ConfigError('unknown action: {}'.format(action_name))
        path, func_name = action
        if not self.quiet:
            sys.stderr.write('generating {}s\n'.format(path.native))
        func_path = action[1].split('.')
        obj = __import__('gen.build.' + '.'.join(func_path[:-1])).build
        for part in func_path:
            obj = getattr(obj, part)
        obj(self)
        self._executed.add(action_name)

    def get_quiet(self):
        return self._quiet

    def set_quiet(self, v):
        self._quiet = v

    quiet = property(get_quiet, set_quiet)
    del get_quiet
    del set_quiet

class BuildConfig(object):
    """Configuration for a specific build."""

    __slots__ = ['project', 'projectconfig', 'os',
                 'variants', 'features', 'libs',
                 'targets']

    def dump(self):
        print('os: {}'.format(self.os))
        for attr in ('variants', 'features', 'libs'):
            print('%}: {}'.format(attr, ' '.join(getattr(self, attr))))

    def _calculate_targets(self):
        self.targets = []
        for t in self.project.targets():
            for v in self.variants:
                vinst = []
                tags = set()
                mods = [t]
                q = list(mods)
                while q:
                    m = q.pop()
                    reqs = set()
                    reqs.update(m.require)
                    for f in m.feature:
                        if f.modid not in self.features:
                            continue
                        for i in f.impl:
                            if all(x in self.libs for x in i.require):
                                reqs.update(i.require)
                    for vv in m.variant:
                        if vv.varname == v:
                            reqs.update(vv.require)
                            vinst.append(vv)
                    reqs.difference_update(tags)
                    tags.update(reqs)
                    for modid in reqs:
                        m = self.project.module_names[modid]
                        q.append(m)
                        mods.append(m)
                assert len(vinst) == 1
                tags.update(self.features)
                tags.update(project.OS[self.os])
                cvars = list(self.project.cvar)
                for m in mods:
                    cvars.extend(m.cvar)
                bt = BuildTarget(t, vinst[0], mods, tags, cvars)
                self.targets.append(bt)

class BuildTarget(object):
    __slots__ = ['target', 'variant', 'modules', 'tags', 'tag_closure',
                 'tag_modules', 'cvars']

    def __init__(self, target, variant, modules, tags, cvars):
        self.target = target
        self.variant = variant
        self.modules = list(modules)
        self.tags = frozenset(tags)
        tag_map = {}
        tag_modules = set()
        for m in self.modules:
            if m.modid is None:
                continue
            tag_modules.add(m.modid)
            if m.require:
                tag_map[m.modid] = frozenset(m.require)
        tag_closure = {}
        for tag in tag_map:
            deps = set()
            ndeps = set(tag_map[tag])
            while ndeps:
                deps.update(ndeps)
                odeps = ndeps
                ndeps = set()
                for dtag in odeps:
                    try:
                        ndeps.update(tag_map[dtag])
                    except KeyError:
                        pass
                ndeps.difference_update(deps)
            deps.discard(tag)
            if deps:
                tag_closure[tag] = frozenset(deps)
        self.tag_closure = tag_closure
        self.tag_modules = tag_modules
        self.cvars = list(cvars)

    def dump(self):
        print('target:', self.target)
        print('variant:', self.variant.varname)
        print('tags:', ' '.join(self.tags))

    def sources(self):
        usedtags = set()
        for m in self.modules:
            base = set(m.require)
            if m.modid is not None:
                base.add(m.modid)
            external = (isinstance(m, project.ExternalLibrary) and
                        m.use_bundled)
            for s in m.sources:
                tags = base.union(s.tags)
                for tag in tuple(tags):
                    try:
                        tags.update(self.tag_closure[tag])
                    except KeyError:
                        pass
                if tags.issubset(self.tags):
                    tags.intersection_update(self.tag_modules)
                    usedtags.update(tags)
                    tags = tuple(sorted(tags))
                    if external:
                        tags = tags + ('.external',)
                    yield Source(s.path, tuple(sorted(tags)))
