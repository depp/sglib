from d3build.module import ExternalModule

def _configure(env):
    return [], {'public': [env.pkg_config('opus')]}

module = ExternalModule(
    name='Opus codec library',
    configure=_configure,
    packages={
        'deb': 'libopus-dev',
        'rpm': 'opus',
        'gentoo': 'media-libs/opus',
        'arch': 'opus',
    }
)
