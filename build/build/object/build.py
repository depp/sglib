from build.error import ConfigError
from build.feedback import Feedback
from build.path import Href
from . import source
import os
import importlib

class Build(object):
    """A build object contains all objects required to build a project.

    A build contains objects, sources, and modules.
    """
    __slots__ = ['_nestsrc', 'sourcemodules', 'sources', 'modules',
                 'targets', 'generated_sources', '_counter',
                 '_names', '_bundles', 'cfg', 'proj',
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

        for mod in proj.modules:
            builder = self.get_builder(mod.type)
            builder(self, mod, mod.name)

        self.sourcemodules, self.sources = \
            source.resolve_sources(self._nestsrc)

    def get_builder(self, mtype):
        try:
            return self._builders[mtype]
        except KeyError:
            raise ConfigError('unknown module type: {!r}'.format(mtype))

    def add_sources(self, name, mod, **kw):
        """Add sources from the given module to the build."""
        if name is None:
            self.cfg.warn('source module has no name')
            return
        if name in self._names:
            raise ConfigError('duplicate name: {}'.format(name))
        self._nestsrc[name] = source.SourceModule.parse(mod, **kw)
        self._names.add(name)

    def add_module(self, name, obj):
        """Add a module to the build."""
        if name is None:
            self.cfg.warn('module has no name')
            return
        self._names.add(name)
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
        if self._lib_dir is None:
            return None
        if self._bundles is None:
            self._bundles = []
            for fname in os.listdir(self.cfg.native_path(self._lib_dir)):
                if fname.startswith('.'):
                    continue
                try:
                    bdir = self._lib_dir.join(fname)
                except ValueError:
                    continue
                if os.path.isdir(self.cfg.native_path(bdir)):
                    self._bundles.append((fname, bdir))
        for fname, path in self._bundles:
            if fname == pattern or fname.startswith(pattern + '-'):
                return path

def build_source(build, mod, name):
    build.add_sources(name, mod)

def build_bundled_library(build, mod, name):
    path = build.find_bundled_library(mod.info.get_string('pattern'))
    if path is None:
        raise ConfigError('library is not bundled: {}'.format(pattern))
    build.add_sources(name, mod.prefix_path(path), external=True)
    return path

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
                path = build_bundled_library(build, submod, name)
            except ConfigError as ex:
                pass
            else:
                fb.write('bundled ({})'.format(path.to_posix()))
                return True
        return False

    def check_nonbundles():
        for submod in nonbundles:
            builder = build.get_builder(submod.type)
            try:
                builder(build, submod, name)
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
            raise ConfigError('could not configure empty module')
        raise ConfigError('could not configure module',
                          suberrors=errs)

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
    'bundled-library': build_bundled_library,
    'executable': generic('executable.Executable'),
    'application-bundle': generic('executable.ApplicationBundle'),

    'multi': build_multi,
    'pkg-config': build_unsupported,
    'sdl-config': build_unsupported,
    'library-search': build_unsupported,
    'framework': build_unsupported,

    'config-header': generic('configheader.ConfigHeader'),
    'version-info': generic('versioninfo.VersionInfo'),
    'literal-file': generic('literalfile.LiteralFile'),
    'template-file': generic('templatefile.TemplateFile'),
}
