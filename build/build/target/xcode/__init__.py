from build.error import ConfigError
from build.path import Path

def target(subtarget, os, cfg, args, archs):
    if archs is None:
        from .. import darwin
        archs = darwin.default_release_archs()
    return Target(archs)

class Target(object):
    __slots__ = ['archs']
    os = 'osx'

    def __init__(self, archs):
        self.archs = tuple(archs)

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
