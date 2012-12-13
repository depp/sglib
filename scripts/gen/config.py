from gen.path import Path
from gen.source import Source
from gen.error import ConfigError
import gen.git as git
import gen.project as project
import os
import sys
import platform

CACHE_FILE = 'config.dat'

# TODO: After loading the project,
# make sure each tag refers to one module

DEFAULT_ACTIONS = {
    'LINUX': ('makefile', 'runner', 'version'),
    'OSX': ('makefile', 'version',),
    'WINDOWS': ('version',),
}

class ProjectConfig(object):
    """Project-wide configuration."""

    __slots__ = [
        'xmlfiles', 'project',

        # Option dictionaries.
        # Options set to None are removed.
        'opt_misc', 'opt_enable', 'opt_bundle', 'opt_env',

        # Map from module ID to (path, bundle).
        'bundles',

        # Cache for get_config
        '_config_cache',
    ]

    def __init__(self):
        self._config_cache = None
        self.load_modules()

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

    def load_modules(self):
        """Load XML project file and module files."""
        import gen.xml as xml

        xmlfiles = [Path('project.xml')]
        proj = xml.load('project.xml', Path())
        paths = list(proj.module_path)
        while paths:
            path = paths.pop()
            for fname in os.listdir(path.native):
                if fname.startswith('.'):
                    continue
                try:
                    fpath = Path(path, fname)
                except ValueError:
                    continue
                if os.path.isdir(fpath.native):
                    paths.append(fpath)
                else:
                    if fname.endswith('.xml'):
                        mod = xml.load(fpath.native, path)
                        proj.add_module(mod)
                        xmlfiles.append(path)

        missing = proj.trim()
        if missing:
            sys.stderr.write(
                'warning: referenced modules do not exist: {}\n'
                .format(', '.join(sorted(missing))))
        proj.validate()
        self.xmlfiles = xmlfiles
        self.project = proj

    def arg_parser(self):
        """Get the argument parser."""
        import argparse
        p = argparse.ArgumentParser(
            description='configure the project')

        def parse_yesno(p, name, dest=None, default=None, help=None,
                        help_neg=None):
            assert dest is not None
            if help_neg is None:
                help_neg = argparse.SUPPRESS
            p.add_argument(
                '--' + name,
                default=default, action='store_true',
                dest=dest, help=help)
            p.add_argument(
                '--no-' + name,
                default=default, action='store_false',
                dest=dest, help=help_neg)

        def parse_enable(p, name, dest=None, default=None, help=None):
            assert dest is not None
            p.add_argument(
                '--enable-' + name,
                default=default, action='store_true',
                dest=dest, help=help)
            p.add_argument(
                '--disable-' + name,
                default=default, action='store_false',
                dest=dest, help=argparse.SUPPRESS)

        def parse_with(p, name, dest=None, default=None, help=None):
            assert dest is not None
            p.add_argument(
                '--with-' + name,
                default=default, action='store_true',
                dest=dest, help=help)
            p.add_argument(
                '--without-' + name,
                default=default, action='store_true',
                dest=dest, help=argparse.SUPPRESS)

        ####################
        # General options

        p.add_argument(
            '--debug', dest='config', default='release',
            action='store_const', const='debug',
            help='use debug configuration')
        p.add_argument(
            '--release', dest='config', default='release',
            action='store_const', const='release',
            help='use release configuration')

        parse_yesno(
            p, 'warnings', dest='warnings',
            help='enable extra compiler warnings',
            help_neg='disable extra compiler warnings')
        parse_yesno(
            p, 'werror', dest='werror',
            help='treat warnings as errors',
            help_neg='do not treat warnings as errors')

        ####################
        # Enable/disable options

        gvar = p.add_argument_group(
            description='build variants:')
        gfeat = p.add_argument_group(
            description='optional features:')
        gmod = p.add_argument_group(
            description='optional packages:')

        for m in self.project.modules:
            if (isinstance(m, project.ExternalLibrary) and
                m.bundled_versions):
                nm = m.modid
                i = nm.rfind('/')
                if i >= 0:
                    nm = nm[i+1:]
                parse_with(
                    gmod, 'bundled-' + nm, dest='bundle:' + m.modid,
                    help='use bundled copy of {}'.format(m.name))

            for c in m.configs():
                if isinstance(c, project.Feature):
                    parse_enable(
                        gfeat, c.flagid, dest='enable:' + c.flagid,
                        help='enable {}'.format(c.name))
                elif isinstance(c, project.Alternative):
                    parse_with(
                        gmod, c.flagid, dest='enable:' + c.flagid,
                        help='use {}'.format(c.name))
                elif isinstance(c, project.Variant):
                    parse_enable(
                        gvar, c.flagid, dest='enable:' + c.flagid,
                        help='build {}'.format(c.name))

        ####################
        # Positional arguments

        p.add_argument('arg', nargs='*', help=argparse.SUPPRESS)

        return p

    def read_opts(self, opts):
        """Read the options from ArgParse.parse_args().

        Returns a list of actions.
        """
        opts = vars(opts)
        misc = {}
        enable = {}
        bundle = {}
        for k, v in opts.items():
            if v is None:
                continue
            i = k.find(':')
            if i >= 0:
                group = k[:i]
                key = k[i+1:]
            else:
                group = None
                key = k
            if group == 'bundle':
                bundle[key] = v
            elif group == 'enable':
                enable[key] = v
            elif group is None:
                misc[key] = v
            else:
                assert False
        args = misc['arg']
        del misc['arg']
        env = {}
        actions = set()
        for arg in args:
            i = arg.find('=')
            if i >= 0:
                env[arg[:i]] = arg[i+1:]
            else:
                actions.append(arg)
        self.opt_misc = misc
        self.opt_enable = enable
        self.opt_bundle = bundle
        self.opt_env = env
        return list(sorted(actions))

    def scan_bundles(self):
        self.bundles = {}

        if self.project.lib_path is None:
            return

        libs = None
        def scan_libdir():
            nonlocal libs
            if libs is not None:
                return
            libs = []
            for path in self.project.lib_path:
                try:
                    fnames = os.listdir(path.native)
                except OSError:
                    continue
                for fname in fnames:
                    npath = os.path.join(path.native, fname)
                    if fname.startswith('.') or not os.path.isdir(npath):
                        continue
                    try:
                        lpath = Path(path, fname)
                    except ValueError:
                        sys.stderr.write(
                            'warning: ignoring {}\n'.format(npath))
                        continue
                    libs.append((fname, lpath))

        for m in self.project.modules:
            if (not isinstance(m, project.ExternalLibrary) or
                not m.bundled_versions or
                not self.opt_bundle.get(m.modid, True)):
                continue
            scan_libdir()
            for v in m.bundled_versions:
                for fname, lpath in libs:
                    i = fname.rfind('-')
                    if i >= 0:
                        libname = fname[:i]
                    else:
                        libname = fname
                    if v.libname == libname:
                        sys.stderr.write(
                            'using {}\n'.format(lpath.native))
                        self.bundles[m.modid] = (lpath, v)
                        break
                else:
                    continue
                break

    def parse_args(self, args):
        """Parse the given command line options.

        Returns a list of actions.
        """
        actions = self.read_opts(self.arg_parser().parse_args(args))
        self.scan_bundles()

    def get_config(self, os):
        """Get the set of enabled flags for the given os.

        Returns (flags, variants), where variants contain all the
        enabled variant flags, and flags contain the other enabled
        flags.
        """

        if self._config_cache is None:
            self._config_cache = {}
        try:
            return self._config_cache[os]
        except KeyError:
            pass

        intrinsics = project.OS[os]

        # Defaults for this platform
        enabled = set()
        for m in self.project.modules:
            for d in m.defaults:
                if d.os is None or os == d.os:
                    enabled.update(d.enable)

        print(', '.join(enabled))

        # User settings override defaults
        uenabled = set()
        for k, v in self.opt_enable.items():
            if v:
                uenabled.add(k)
            else:
                enabled.discard(k)

        # Disable defaults that conflict with user settings
        for c in self.project.configs():
            if not isinstance(c, project.Alternatives):
                continue
            if any(alt.flagid in uenabled for alt in c.alternatives):
                enabled.difference_update(
                    alt.flagid for alt in c.alternatives)
        enabled.update(uenabled)

        # Limit flags to those actually reachable
        # Set aside list of variants
        # Check for unsatisfied / oversatisfied alternatives
        reachable = set()
        variants = set()
        for c in self.project.configs(enabled.union(intrinsics)):
            if isinstance(c, project.Variant):
                variants.add(c.flagid)
                continue
            try:
                flagid = c.flagid
            except AttributeError:
                pass
            else:
                reachable.add(flagid)
            if isinstance(c, project.Alternatives):
                alts = [alt for alt in c.alternatives
                        if alt.flagid in enabled]
                if not alts:
                    raise ConfigError(
                        'unsatisfied alternative',
                        'none of the following alternatives are enabled:\n'
                        + ''.join('  --with-{}\n'.format(alt.flagid)
                                  for alt in c.alternatives))
                if len(alts) > 1:
                    raise ConfigError(
                        'conflicting settings',
                        'the following settings conflict:\n'
                        + ''.join('  --with-{}\n'.format(alt.flagid)
                                  for alt in alts))

        if not variants:
            raise ConfigError('no build targets are enabled')
        reachable.update(intrinsics)
        result = frozenset(reachable), frozenset(variants)
        self._config_cache[os] = result
        return result

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
                'makefile': (Path('Makefile'), 'make.gen_makefile'),
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
            sys.stderr.write('generating {}\n'.format(path.native))
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


def configure(argv):
    config = ProjectConfig()
    config.parse_args(argv)
    for os in ('linux', 'osx', 'windows'):
        enabled, variants = config.get_config(os)
        print('{}-enabled: {}'.format(os, ', '.join(enabled)))
        print('{}-variants: {}'.format(os, ', '.join(variants)))
    print('STOPPING HERE')
    sys.exit(1)

def reconfigure(argv):
    pass
