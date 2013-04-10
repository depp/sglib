from build.error import ConfigError
from build.target.feedback import Feedback
from . import generic
import build.data as data
import sys
import os

MODULES = {
    'source', 'executable', 'application-bundle',
    'multi', 'pkg-config', 'library-search', 'framework',
    'config-header', 'version-info',
}

def parse_exe_args(info):
    args = []
    pfx = 'default-args.'
    for k, v in info.items():
        if not k.startswith(pfx):
            continue
        k = int(k[len(pfx):])
        args.append((k, v))
    args.sort(key=lambda x: x[0])
    return [arg[1] for arg in args]

class ExeCommon(object):
    __slots__ = ['name', 'source', 'filename', 'args']
    is_target = True

    def __init__(self, name, source, filename, args):
        self.name = name
        self.source = source
        self.filename = filename
        self.args = args

class ExeModule(ExeCommon):
    __slots__ = []

class ApplicationBundleModule(ExeCommon):
    __slots__ = []

class Build(object):
    __slots__ = ['targets', 'modules', 'sources', 'proj']
    def __init__(self, proj):
        self.targets = []
        self.modules = {}
        self.sources = None
        self.proj = proj
    def add(self, mod):
        if mod.is_target:
            self.targets.append(mod)
        if mod.name is not None:
            assert mod.name not in self.modules
            self.modules[mod.name] = mod

class Target(object):
    __slots__ = ['subtarget', '_bundles', 'bundled_libs']

    def __init__(self, subtarget, osname, args):
        self.subtarget = subtarget
        self._bundles = None
        self.bundled_libs = args.bundled_libs

    def build_module(self, proj, mod, *, name=None):
        if mod.type not in MODULES:
            raise ConfigError('unknown module type: {!r}'
                              .format(mod.type))
        try:
            func = getattr(self, 'mod_' + mod.type.replace('-', '_'))
        except AttributeError:
            raise ConfigError('target does not support module type: {}'
                              .format(mod.type))
        if name is None:
            name = mod.name
        return func(proj, mod, name)

    def gen_build(self, proj):
        self._bundles = []
        if 'lib-dir' in proj.info:
            lib_dir = proj.info.get_path('lib-dir')
            for fname in os.listdir(proj.native(lib_dir)):
                if fname.startswith('.'):
                    continue
                try:
                    bdir = lib_dir.join(fname)
                except ValueError:
                    continue
                self._bundles.append((fname, bdir))
        build = Build(proj)
        for mod in proj.modules:
            for rmod in self.build_module(proj, mod):
                build.add(rmod)
        generic.resolve_sources(build)
        return build

    def mod_source(self, proj, mod, name):
        smod = generic.SourceModule.from_module(mod, name)
        return [smod]

    def get_bundled_library(self, proj, mod, name):
        pattern = mod.info.get_string('pattern')
        for fname, path in self._bundles:
            if fname == pattern or fname.startswith(pattern + '-'):
                break
        else:
            return None
        return generic.SourceModule.from_module(
            mod.prefix_path(path), name, external=True), path

    def mod_bundled_library(self, proj, mod, name):
        r = self.get_bundled_library(proj, mod, name)
        if r is None:
            raise ConfigError('library is not bundled with this project')
        return [r[0]]

    def mod_multi(self, proj, mod, name):
        desc = mod.info.get_string('desc', '<unknown library>')
        bundles = []
        nonbundles = []
        for submod in mod.modules:
            if submod.type == 'bundled-library':
                bundles.append(submod)
            else:
                nonbundles.append(submod)
        def check_bundles():
            for submod in bundles:
                r = self.get_bundled_library(proj, submod, name)
                if r is not None:
                    fb.write('bundled ({})'.format(r[1].to_posix()))
                    return [r[0]]
        def check_nonbundles():
            for submod in nonbundles:
                try:
                    return list(self.build_module(proj, submod, name=name))
                except ConfigError as ex:
                    errs.append(ex)
        errs = []
        with Feedback('Checking for {}...'.format(desc)) as fb:
            if self.bundled_libs == 'auto':
                order = [check_nonbundles, check_bundles]
            elif self.bundled_libs == 'yes':
                order = [check_bundles, check_nonbundles]
            else:
                order = [check_nonbundles]
            r = None
            for func in order:
                r = func()
                if r is not None:
                    return r
            if not errs:
                raise ConfigError('could not configure empty module')
            raise ConfigError('could not configure module',
                              suberrors=errs)

    def mod_config_header(self, proj, mod, name):
        from .configheader import ConfigHeader
        target = mod.info.get_path('target')
        return [ConfigHeader(name, target, proj.environ['flag'])]

    def mod_version_info(self, proj, mod, name):
        from .versioninfo import VersionInfo
        target = mod.info.get_path('target')
        repos = {}
        for k in mod.info:
            if k.startswith('repo.'):
                repos[k[5:]] = proj.native(mod.info.get_path(k))
        return [VersionInfo(name, target, repos, 'git')]

    def mod_executable(self, proj, mod, name):
        smod = generic.SourceModule.from_module(mod, proj.gen_name())
        filename = mod.info.get_string('filename')
        args = parse_exe_args(mod.info)
        return (smod, ExeModule(name, smod.name, filename, args))

    def mod_application_bundle(self, proj, mod, name):
        smod = generic.SourceModule.from_module(mod, proj.gen_name())
        filename = mod.info.get_string('filename')
        args = parse_exe_args(mod.info)
        amod = ApplicationBundleModule(name, smod.name, filename, args)
        return (smod, amod)
