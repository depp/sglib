def target(subtarget, os, cfg, args):
    return Target()

class Target(object):
    __slots__ = []
    os = 'windows'

    def gen_build(self, cfg, proj):
        from build.object import build
        from . import project
        filename = proj.filename
        build = build.Build(cfg, proj, {})
        for target in build.targets:
            project.make_target(build, target)
        return build
