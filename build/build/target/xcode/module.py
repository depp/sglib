from .. import nix
from build.object import env

def build_framework(build, mod, name, external):
    fwname = mod.info.get_string('filename')
    build.add(name, env.EnvModule({'FRAMEWORKS': (fwname,)}))

BUILDERS = {
    'framework': build_framework,
}

CONFIG_BUILDERS = dict(nix.BUILDERS)
CONFIG_BUILDERS.update(BUILDERS)
