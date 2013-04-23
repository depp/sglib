from build.path import Path

def version(x):
    i = x.index('.')
    return int(x[:i]), int(x[i+1:])

def target(subtarget, os, cfg, vars, archs):
    from .. import darwin
    from build.param import ParamParser
    osx_version = darwin.osx_version() or (10, 5)
    p = ParamParser()
    p.add_param('version', type=version, default=osx_version)
    p.add_param('config', type='bool', default=(cfg.config == 'debug'))
    params = p.parse(subtarget)
    if archs is None:
        archs = darwin.default_release_archs(params['version'])
    if params['config']:
        from .. import nix
        config_env = nix.default_env(cfg, vars, 'osx')
    else:
        config_env = None
    return Target(archs, params['version'], config_env)

class Target(object):
    __slots__ = ['archs', 'version', 'config_env']
    os = 'osx'

    def __init__(self, archs, version, config_env):
        self.archs = tuple(archs)
        self.version = version
        self.config_env = config_env

    def gen_build(self, cfg, proj):
        from . import project
        from build.object import build
        filename = proj.filename
        if self.config_env is None:
            builders = BUILDERS
        else:
            from .. import nix
            builders = dict(nix.BUILDERS)
            builders.update(BUILDERS)
        build = build.Build(cfg, proj, builders)
        xcproj = project.Project(cfg)
        xcpath = Path('/', 'builddir').join1(filename, '.xcodeproj')
        xcproj.add_build(build, xcpath)
        return build

class Framework(object):
    __slots__ = ['framework_name']
    def __init__(self, framework_name):
        self.framework_name = framework_name

def build_framework(build, mod, name, external):
    build.add_module(name, Framework(mod.info.get_string('filename')))

BUILDERS = {
    'framework': build_framework,
}
