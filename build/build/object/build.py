from build.error import ProjectError, ConfigError
from build.feedback import Feedback
from build.path import Href
import build.project.data as data
from . import source, target, env, GeneratedSource
import os
import importlib
import sys

class MissingLibrary(ConfigError):
    __slots__ = ['mod']
    def __init__(self, *args, mod, **kw):
        super(MissingLibrary, self).__init__(*args, **kw)
        self.mod = mod

# There is a certain amount of guessing here...
LIBRARY_HELP = [
    ('linux:debian linux:ubuntu linux:mint',
     'pkg-deb', 'sudo apt-get install {}'),

    ('linux:fedora linux:redhat linux:centos',
     'pkg-rpm', 'sudo yum install {}'),

    ('linux:gentoo',
     'pkg-gentoo', 'sudo emerge {}'),

    ('linux:arch',
     'pkg-arch', 'sudo pacman -S {}'),
]

def format_missinglibs(libs):
    import io, platform
    info = io.StringIO()
    print(file=info)
    print('Development files for the following libraries could not be found:',
          file=info)
    for lib in libs:
        desc = lib.info.get_string('desc', '<unknown library>')
        print('  * {}'.format(desc), file=info)
    if platform.system() == 'Linux':
        distro = platform.linux_distribution(
            full_distribution_name=False)[0]
        plat = 'linux:' + distro.lower()
    else:
        plat = None
    for plats, key, cmd in LIBRARY_HELP:
        if plat not in plats.split():
            continue
        pkgs = [lib.info.get_string(key) for lib in libs if key in lib.info]
        print(file=info)
        print('To install the missing libraries, run the following command:',
              file=info)
        print(file=info)
        print('  {}'.format(cmd.format(' '.join(pkgs))), file=info)
        print(file=info)
        break
    return info.getvalue()

class Build(object):
    """A build object contains all objects required to build a project.

    A build contains objects, sources, and modules.
    """
    __slots__ = ['sources', 'modules', 'bundles', 'cfg', 'proj',
                 '_counter', '_bundles', '_lib_dir', '_builders']

    def __init__(self, cfg, proj, builders):
        # self.sources
        self.modules = {}
        self.bundles = set()
        self.cfg = cfg
        self.proj = proj
        self._counter = 0
        self._bundles = None
        self._lib_dir = proj.info.get_path('lib-dir', None)
        self._builders = dict(MODULE_TYPES)

        for k, v in builders.items():
            if k not in self._builders:
                raise ValueError('cannot add builder for new tag: {!r}'
                                 .format(k))
            if self._builders[k] is not None:
                raise ValueError('cannot override builder for tag: {}'
                                 .format(k))
            self._builders[k] = v

        errors = 0
        missinglibs = []
        for mod in proj.modules:
            try:
                builder = self.get_builder(mod.type)
                builder(self, mod, mod.name, False)
            except ConfigError as ex:
                if isinstance(ex, MissingLibrary):
                    missinglibs.append(ex.mod)
                errors += 1
                ex.write(sys.stderr)
                sys.stderr.write('\n')

        srcmodules = {k: v for k, v in self.modules.items()
                      if isinstance(v, source.SourceModule)}
        srcmodules, self.sources = source.resolve_sources(srcmodules)
        self.modules.update(srcmodules)

        if errors > 0:
            if missinglibs:
                details = format_missinglibs(missinglibs)
            else:
                details = None
            raise ConfigError('configuration failed', details)

    def get_builder(self, mtype):
        """Get the builder function for a given module type."""
        try:
            builder = self._builders[mtype]
        except KeyError:
            raise ProjectError('unknown module type: {!r}'.format(mtype))
        if builder is None:
            raise ConfigError('module type is unsupported on this target: {}'
                              .format(mtype))
        return builder

    def add(self, name, obj):
        """Add a module to the build."""
        if name is None:
            name = self.gen_name()
        if name in self.modules:
            raise ProjectError(
                'duplicate module name: {}'.format(name))
        self.modules[name] = obj

    def gen_name(self):
        """Generate a unique name."""
        self._counter += 1
        return Href(None, 'build-{}'.format(self._counter))

    def find_bundled_library(self, pattern):
        """Find the path for a bundle with the given pattern."""
        if self._lib_dir is None or not self.cfg.exists(self._lib_dir):
            return None
        if self._bundles is None:
            self._bundles = []
            for fname in os.listdir(self.cfg.native_path(self._lib_dir)):
                if fname.startswith('.'):
                    continue
                try:
                    bdir = self._lib_dir.join1(fname)
                except ValueError:
                    continue
                if os.path.isdir(self.cfg.native_path(bdir)):
                    self._bundles.append((fname, bdir))
        for fname, path in self._bundles:
            if fname == pattern or fname.startswith(pattern + '-'):
                return path

    def targets(self):
        for module in list(self.modules.values()):
            if isinstance(module, target.Target):
                yield module

    def generated_sources(self):
        for module in list(self.modules.values()):
            if isinstance(module, GeneratedSource):
                yield module

    def get_env(self, base_env, deps, **kw):
        """Get a build environment, given a list of modules.

        Raises an error if any module specified is not an EnvModule.
        Extra environment variables can be specified as keyword
        arguments.
        """
        build_env = []
        if base_env is not None:
            build_env.append(base_env)
        for req in deps:
            dep = self.modules[req]
            if not isinstance(dep, env.EnvModule):
                raise ProjectError(
                    'cannot understand dependency on module: {}'
                    .format(req))
            build_env.append(dep.env)
        build_env.append(kw)
        return env.merge_env(build_env)

def build_source(build, mod, name, external):
    build.add(name, source.SourceModule.parse(mod, external=external))

def build_null(build, mod, name, external):
    build.add(name, source.SourceModule())

def build_bundled_library(build, mod, name, external):
    pattern = mod.info.get_string('pattern')
    path = build.find_bundled_library(pattern)
    if path is None:
        raise ConfigError('library is not bundled: {}'.format(pattern))
    mod = mod.prefix_path(path)
    errs = []
    gen_subname = build.gen_name()
    for submod in mod.modules:
        subname = gen_subname if submod.name is None else submod.name
        try:
            builder = build.get_builder(submod.type)
            builder(build, submod, subname, True)
            break
        except ConfigError as ex:
            errs.append(ex)
    else:
        if not errs:
            raise ConfigError('could not configure empty bundled library')
        raise ConfigError('could not configure bundled library',
                          suberrors=errs)
    lmod = source.SourceModule.parse(mod, sources='error')
    lmod.private_modules.add(subname)
    build.add(name, lmod)
    return '{} {}'.format(submod.type, path.to_posix())

def build_multi(build, mod, name, external):
    desc = mod.info.get_string('desc', '<unknown library>')
    bundles = []
    nonbundles = []
    for submod in mod.modules:
        if submod.name is not None:
            build.cfg.warn(
                'ignoring name inside multi module: {}'
                .format(submod.name))
        if submod.type == 'bundled-library':
            bundles.append(submod)
        else:
            nonbundles.append(submod)

    def check_bundles():
        for submod in bundles:
            try:
                ident = build_bundled_library(build, submod, name, external)
            except ConfigError as ex:
                pass
            else:
                fb.write('bundled ({})'.format(ident))
                return True
        return False

    def check_nonbundles():
        for submod in nonbundles:
            try:
                builder = build.get_builder(submod.type)
                builder(build, submod, name, external)
                if submod.type == 'null':
                    fb.write('not needed')
                return True
            except ConfigError as ex:
                errs.append(ex)
        return False

    errs = []
    with Feedback(build.cfg, 'Checking for {}...'.format(desc)) as fb:
        if build.cfg.bundled_libs == 'auto':
            order = [check_nonbundles, check_bundles]
        elif build.cfg.bundled_libs == 'yes':
            order = [check_bundles, check_nonbundles]
        else:
            order = [check_nonbundles]
        if any(func() for func in order):
            return
        if not errs:
            raise MissingLibrary('could not configure empty module',
                                 mod=mod)
        raise MissingLibrary('could not configure module',
                             suberrors=errs, mod=mod)

def generic(class_name):
    func = None
    def wrapper(*args):
        nonlocal func
        if func is None:
            i = class_name.rindex('.')
            mod_name = class_name[:i]
            obj_name = class_name[i+1:]
            mod = importlib.import_module('build.object.' + mod_name)
            func = getattr(mod, obj_name).build
        func(*args)
    return wrapper

MODULE_TYPES = {
    'source': build_source,
    'null': build_null,
    'bundled-library': build_bundled_library,
    'executable': generic('target.Executable'),
    'application-bundle': generic('target.ApplicationBundle'),

    'multi': build_multi,
    'pkg-config': None,
    'sdl-config': None,
    'library-search': None,
    'framework': None,
    'msvc': None,

    'config-header': generic('configheader.ConfigHeader'),
    'version-info': generic('versioninfo.VersionInfo'),
    'literal-file': generic('literalfile.LiteralFile'),
    'template-file': generic('templatefile.TemplateFile'),
}
