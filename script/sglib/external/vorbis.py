from d3build.module import ExternalModule

def _configure(env):
    return [], {'public': [env.pkg_config('vorbis')]}

module = ExternalModule(
    name='Vorbis codec library',
    configure=_configure,
    packages={
        'deb': 'libvorbis-dev',
        'rpm': 'libvorbis-devel',
        'gentoo': 'media-libs/libvorbis',
        'arch': 'libvorbis',
    }
)
