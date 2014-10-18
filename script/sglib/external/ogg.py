from d3build.module import ExternalModule

def _configure(env):
    return [], {'public': [env.pkg_config('ogg')]}

module = ExternalModule(
    name='Ogg bitstream library',
    configure=_configure,
    packages={
        'deb': 'libogg-dev',
        'rpm': 'libogg-devel',
        'gentoo': 'media-libs/libogg',
        'arch': 'libogg',
    }
)
