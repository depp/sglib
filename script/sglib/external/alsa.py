from d3build.module import ExternalModule

def _configure(env):
    return [], {'public': [env.pkg_config('alsa')]}

module = ExternalModule(
    name='ALSA',
    configure=_configure,
    packages={
        'deb': 'libasound2-dev',
        'rpm': 'alsa-lib-devel',
        'gentoo': 'media-libs/alsa-lib',
        'arch': 'alsa-lib',
    }
)
