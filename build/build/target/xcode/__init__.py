from build.error import ConfigError
from build.path import Path

def version(x):
    i = x.index('.')
    return int(x[:i]), int(x[i+1:])

def target(subtarget, os, cfg, args, archs):
    from .. import darwin
    from build.param import ParamParser
    osx_version = darwin.osx_version() or (10, 5)
    p = ParamParser()
    p.add_param('version', type=version, default=osx_version)
    params = p.parse(subtarget)
    if archs is None:
        archs = darwin.default_release_archs(params['version'])
    return Target(archs, params['version'])

class Target(object):
    __slots__ = ['archs', 'version']
    os = 'osx'

    def __init__(self, archs, version):
        self.archs = tuple(archs)
        self.version = version

    def gen_build(self, cfg, proj):
        from . import project
        from build.object import build
        filename = proj.filename
        build = build.Build(cfg, proj, BUILDERS)
        xcproj = project.Project(cfg)
        xcpath = Path('/', 'builddir').join1(filename, '.xcodeproj')
        xcproj.add_build(build, xcpath)
        return build

class Framework(object):
    __slots__ = ['framework_name']
    def __init__(self, framework_name):
        self.framework_name = framework_name

def build_framework(build, mod, name):
    build.add_module(name, Framework(mod.info.get_string('filename')))

BUILDERS = {
    'framework': build_framework,
}
