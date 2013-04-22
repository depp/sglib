from build.error import ConfigError
from build.feedback import Feedback
from build.path import Href
import build.project.data as data
from . import source
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
    __slots__ = ['_nestsrc', 'sourcemodules', 'sources', 'modules',
                 'targets', 'generated_sources', '_counter',
                 '_names', '_bundles', 'bundles', 'cfg', 'proj',
                 '_lib_dir', '_builders']

    def __init__(self, cfg, proj, builders):
        self._nestsrc = {}
        self.sourcemodules = None
        self.sources = None
        self.modules = {}
        self.targets = []
        self.generated_sources = {}
        self._counter = 0
        self._names = set()
        self._bundles = None
        self.bundles = set()
        self.cfg = cfg
        self.proj = proj
        self._lib_dir = proj.info.get_path('lib-dir', None)
        self._builders = dict(MODULE_TYPES)

        for k, v in builders.items():
            if k not in self._builders:
                raise ValueError('cannot add builder for new tag: {!r}'
                                 .format(k))
            if self._builders[k] is not build_unsupported:
                raise ValueError('cannot override builder for tag: {}'
                                 .format(k))
            self._builders[k] = v

        errors = 0
        missinglibs = []
        for mod in proj.modules:
            try:
                builder = self.get_builder(mod.type)
                builder(self, mod, mod.name)
            except ConfigError as ex:
                if isinstance(ex, MissingLibrary):
                    missinglibs.append(ex.mod)
                errors += 1
                ex.write(sys.stderr)
                sys.stderr.write('\n')

        self.sourcemodules, self.sources = \
            source.resolve_sources(self._nestsrc)

        if errors > 0:
            if missinglibs:
                details = format_missinglibs(missinglibs)
            else:
                details = None
            raise ConfigError('configuration failed', details)

    def get_builder(self, mtype):
        try:
            return self._builders[mtype]
        except KeyError:
            raise ConfigError('unknown module type: {!r}'.format(mtype))

    def _add_name(self, name):
        if name is None:
            self.cfg.warn('module has no name')
            return
        if name in self._names:
            raise ConfigError('duplicate name: {}'.format(name))
        self._names.add(name)

    def add_srcmodule(self, name, obj):
        """Add a source module to the build."""
        self._add_name(name)
        self._nestsrc[name] = obj

    def add_sources(self, name, mod):
        """Add sources from the given module to the build."""
        self.add_srcmodule(name, source.SourceModule.parse(mod))

    def add_module(self, name, obj):
        """Add a module to the build."""
        self._add_name(name)
        self.modules[name] = obj

    def add_target(self, obj):
        """Add a target to the build."""
        self.targets.append(obj)

    def add_generated_source(self, obj):
        """Add a generated source to the build."""
        if obj.target in self.generated_sources:
            raise ConfigError(
                'multiple generated sources for path: {}'.format(obj.target))
        self.generated_sources[obj.target] = obj

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

def build_source(build, mod, name):
    build.add_srcmodule(name, source.SourceModule.parse(mod))

def build_null(build, mod, name):
    build.add_srcmodule(name, source.SourceModule())

def build_bundled_library(build, mod, name):
    pattern = mod.info.get_string('pattern')
    path = build.find_bundled_library(pattern)
    if path is None:
        raise ConfigError('library is not bundled: {}'.format(pattern))
    mod = mod.prefix_path(path)
    errs = []
    gen_subname = build.gen_name()
    for submod in mod.modules:
        subname = gen_subname if submod.name is None else submod.name
        builder = build.get_builder(submod.type)
        try:
            builder(build, submod, subname)
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
    build.add_srcmodule(name, lmod)
    return '{} {}'.format(submod.type, path.to_posix())

def build_multi(build, mod, name):
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
                ident = build_bundled_library(build, submod, name)
            except ConfigError as ex:
                pass
            else:
                fb.write('bundled ({})'.format(ident))
                return True
        return False

    def check_nonbundles():
        for submod in nonbundles:
            builder = build.get_builder(submod.type)
            try:
                builder(build, submod, name)
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

def build_unsupported(build, mod, name):
    raise ConfigError(
        'module type is unsupported on this target: {}'.format(name))

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
    'executable': generic('executable.Executable'),
    'application-bundle': generic('executable.ApplicationBundle'),

    'multi': build_multi,
    'pkg-config': build_unsupported,
    'sdl-config': build_unsupported,
    'library-search': build_unsupported,
    'framework': build_unsupported,
    'msvc': build_unsupported,

    'config-header': generic('configheader.ConfigHeader'),
    'version-info': generic('versioninfo.VersionInfo'),
    'literal-file': generic('literalfile.LiteralFile'),
    'template-file': generic('templatefile.TemplateFile'),
}
