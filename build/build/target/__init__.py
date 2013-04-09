from build.error import ConfigError
from build.target.feedback import Feedback
from . import generic
import build.data as data
import sys

MODULES = {
    'source', 'executable',
    'multi', 'pkg-config', 'library-search', 'framework',
    'config-header', 'version-info',
}

class Build(object):
    __slots__ = ['targets', 'modules', 'sources']
    def __init__(self):
        self.targets = []
        self.modules = {}
        self.sources = None
    def add(self, mod):
        if mod.is_target:
            self.targets.append(mod)
        if mod.name is not None:
            assert mod.name not in self.modules
            self.modules[mod.name] = mod

class Target(object):
    __slots__ = ['subtarget']

    def __init__(self, subtarget, osname, args):
        self.subtarget = subtarget

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
        build = Build()
        for mod in proj.modules:
            for rmod in self.build_module(proj, mod):
                build.add(rmod)
        generic.resolve_sources(build)
        return build

    def mod_source(self, proj, mod, name):
        smod = generic.SourceModule.from_module(mod)
        return [smod]

    def mod_multi(self, proj, mod, name):
        desc = mod.info.get_string('desc', '<unknown library>')
        with Feedback('Checking for {}...'.format(desc)) as fb:
            errs = []
            for submod in mod.modules:
                try:
                    return list(self.build_module(proj, submod, name=name))
                except ConfigError as ex:
                    errs.append(ex)
                else:
                    assert result is not None
                    break
            if not errs:
                raise ConfigError('could not configure empty module')
            raise ConfigError('could not configure module', suberrors=errs)

    def mod_config_header(self, proj, mod, name):
        return ()

    def mod_version_info(self, proj, mod, name):
        return ()
