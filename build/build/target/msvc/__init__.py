from build.error import ConfigError

def target(subtarget, os, cfg, args, archs):
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
        from . import project
        filename = proj.filename
        build = build.Build(cfg, proj, {})
        for target in build.targets:
            project.make_target(build, target)
        return build
