from build.error import ConfigError

XMLNS = 'http://schemas.microsoft.com/developer/msbuild/2003'
CONFIGS = ('Debug', 'Release')
PLATS = ('Win32', 'x64')

def target(subtarget, os, cfg, vars, archs):
    if archs is not None:
        raise ConfigError(
            'architecture cannot be specified for this target')
    return Target()

class Target(object):
    __slots__ = []
    os = 'windows'
    archs = None

    def gen_build(self, cfg, proj):
        from build.object import build
        from . import project, solution, module
        filename = proj.filename
        build = build.Build(cfg, proj, module.BUILDERS)
        for target in build.targets:
            project.make_target(build, target)
        solution.make_solution(build, proj)
        return build
