from d3build.module import ExternalModule

def _configure(env):
    return [], {'public': [env.pkg_config('libpng12')]}

module = ExternalModule(
    name='LibPNG',
    configure=_configure,
    packages={
        'deb': 'libpng-dev',
        'rpm': 'libpng-devel',
        'gentoo': 'media-libs/libpng',
        'arch': 'libpng',
    }
)
