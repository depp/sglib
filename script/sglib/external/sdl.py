from d3build.module import ExternalModule

def _configure(version):
    def configure(env):
        return [], {'public': [env.sdl_config(version)]}
    return configure

version_1 = ExternalModule(
    name='Ogg bitstream library',
    configure=_configure(1),
    packages={
        'deb': 'libsdl1.2-dev',
        'rpm': 'SDL-devel',
        'gentoo': 'media-libs/libsdl',
        'arch': 'sdl',
    }
)

version_2 = ExternalModule(
    name='LibSDL 2',
    configure=_configure(2),
    packages={
        'deb': 'libsdl2-dev',
        'rpm': 'SDL2-devel',
        'gentoo': 'media-libs/libsdl2',
        'arch': 'sdl2',
    }
)
