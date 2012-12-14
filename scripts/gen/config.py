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
    'linux': ('makefile', 'runner', 'version'),
    'osx': ('makefile', 'version',),
    'windows': ('version',),
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

        '_repos', '_versions', '_actions', '_native_os', '_executed',
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
        if actions:
            return actions
        return DEFAULT_ACTIONS[self.native_os]

    def get_config(self, os):
        """Get the set of enabled flags for the given os.

        Returns (flags, variants), where variants contain all the
        enabled variant flags, and flags contain the other enabled
        flags.
        """

        if getattr(self, '_config_cache', None) is None:
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

        reachable.update(intrinsics)

        # Check for require.enable
        for c in self.project.configs(enabled.union(intrinsics)):
            if isinstance(c, project.Require):
                for flagid in c.enable:
                    if flagid not in reachable:
                        raise ConfigError(
                            'unsatisfied dependency on {} flag'
                            .format(flagid))

        if not variants:
            raise ConfigError('no build targets are enabled')
        result = BuildConfig(os, self, reachable, variants)
        self._config_cache[os] = result
        return result

    @property
    def repos(self):
        if getattr(self, '_repos', None) is None:
            if self.project is None:
                raise AttributeError('repos')
            self._repos = {
                'APP': Path(),
                'SG': self.project.module_names['sg'].module_root(),
            }
        return self._repos

    @property
    def versions(self):
        if getattr(self, '_versions', None) is None:
            if self.project is None:
                raise AttributeError('repos')
            versions = {}
            for k, v in self.repos.items():
                versions[k] = git.get_info(self, v)
            self._versions = versions
        return self._versions

    @property
    def actions(self):
        if getattr(self, '_actions', None) is None:
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
        os = getattr(self, '_native_os', None)
        if os is None:
            d = {
                'Darwin': 'osx',
                'Linux': 'linux',
                'Windows': 'windows',
            }
            s = platform.system()
            try:
                os = d[s]
            except KeyError:
                raise ConfigError('unsupported platform: {}'.format(s))
            self._native_os = os
        return os

    def exec_action(self, action_name, quiet):
        if getattr(self, '_executed', None) is None:
            self._executed = set()
        if action_name in self._executed:
            return
        actions = self.actions
        try:
            action = actions[action_name]
        except KeyError:
            raise ConfigError('unknown action: {}'.format(action_name))
        path, func_name = action
        if not quiet:
            sys.stderr.write('generating {}\n'.format(path.native))
        func_path = action[1].split('.')
        obj = __import__('gen.build.' + '.'.join(func_path[:-1])).build
        for part in func_path:
            obj = getattr(obj, part)
        obj(self)
        self._executed.add(action_name)

class BuildConfig(object):
    """Configuration for a specific os."""

    __slots__ = [
        'config', 'enabled', 'variants', '_targets', '_all_modules',
        'os',
    ]

    def __init__(self, os, config, enabled, variants):
        self.os = os
        self.config = config
        self.enabled = frozenset(enabled)
        self.variants = frozenset(variants)
        self._targets = None
        self._all_modules = None

    def targets(self):
        if self._targets is not None:
            return self._targets
        targets = []
        proj = self.config.project
        variants = [c for c in proj.configs()
                    if isinstance(c, project.Variant)
                    and c.flagid in self.variants]
        for target in proj.targets():
            for variant in variants:
                enabled = set(self.enabled)
                enabled.add(variant.flagid)
                mods, unsat = proj.closure([target], enabled)
                if unsat:
                    raise ConfigError(
                        'could not satisfy dependency on modules: {}'
                        .format(', '.join(sorted(unsat))))
                targets.append(BuildTarget(
                    self, target, variant, enabled, mods))
        self._targets = targets
        return targets

    def all_modules(self):
        if self._all_modules is not None:
            return self._all_modules
        allmods = set()
        for t in self.targets():
            allmods.update(t.module_names)
        allmods = frozenset(allmods)
        self._all_modules = allmods
        return allmods

class BuildTarget(object):
    __slots__ = [
        'buildconfig', 'target', 'variant', 'enabled', 'modules',
        'module_names', '_cvars', '_module_closure',
    ]

    def __init__(self, buildconfig, target, variant, enabled, modules):
        self.buildconfig = buildconfig
        self.target = target
        self.variant = variant
        self.enabled = frozenset(enabled)
        self.modules = tuple(modules)
        self.module_names = {}
        for m in self.modules:
            if m.modid is not None:
                self.module_names[m.modid] = m
        self._cvars = None
        self._module_closure = None

    @property
    def cvars(self):
        if self._cvars is not None:
            return self._cvars
        cvars = list(self.buildconfig.config.project.cvar)
        for m in self.modules:
            cvars.extend(m.cvar)
        cvars = tuple(cvars)
        self._cvars = cvars
        return cvars

    def _get_module_closure(self):
        """Get the closure of public module dependencies."""
        if self._module_closure is not None:
            return self._module_closure
        proj = self.buildconfig.config.project
        reqmap = {}
        for m in self.modules:
            # 'modid is None' is fine, that's the one and only target
            reqs = set()
            for c in m.configs(self.enabled):
                if isinstance(c, project.Require) and c.public:
                    reqs.update(c.modules)
            if reqs:
                reqmap[m.modid] = reqs
        req_closure = {}
        for k, v in reqmap.items():
            reqs = set()
            nreqs = set(v)
            while nreqs:
                t = set()
                for r in nreqs:
                    try:
                        t.update(reqmap[r])
                    except KeyError:
                        pass
                reqs.update(nreqs)
                t.difference_update(reqs)
                nreqs = t
            req_closure[k] = tuple(reqs)
        self._module_closure = req_closure
        return req_closure

    def sources(self):
        """Iterate over all sources in the target."""
        module_closure = self._get_module_closure()
        for m in self.modules:
            for source in m.sources:
                if not self.enabled.issuperset(source.enable):
                    continue
                mod = set(source.module)
                mod.add(m.modid)
                for modid in tuple(mod):
                    try:
                        mod.update(module_closure[modid])
                    except KeyError:
                        pass
                mod.discard(None)
                yield Source(source.path, (), mod)

    def exe_name(self, machine=None):
        os = self.buildconfig.os
        a = [self.target.exe_name[os]]
        if machine:
            a.append(machine)
        v = self.variant.exe_suffix[os]
        if v:
            a.append(v)
        if os == 'linux':
            return '_'.join(a)
        else:
            return ' '.join(a)

def configure(argv):
    config = ProjectConfig()
    actions = config.parse_args(argv)
    return config, actions

def reconfigure(argv):
    config = ProjectConfig()
    config.parse_args(argv)
    return config
