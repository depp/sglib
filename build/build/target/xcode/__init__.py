from build.error import ConfigError
from build.path import Path

def target(subtarget, os, cfg, args):
    return Target()

class Target(object):
    __slots__ = []
    os = 'osx'

    def gen_build(self, cfg, proj):
        from . import project
        from build.object import build
        if 'filename' in proj.info:
            filename = proj.info.get_string('filename')
        elif 'name' in proj.info:
            filename = proj.info.get_string('name')
        else:
            raise ConfigError('project lacks name or filename attribute')
        from build.object.build import Build
        build = build.Build(cfg, proj, BUILDERS)
        xcproj = project.Project(cfg)
        xcpath = Path('/', 'builddir').join(filename + '.xcodeproj')
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
