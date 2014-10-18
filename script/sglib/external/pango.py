from d3build.module import ExternalModule

def _configure(env):
    return [], {'public': [env.pkg_config('pangocairo')]}

module = ExternalModule(
    name='Pango',
    configure=_configure,
    packages={
        'deb': 'libpango1.0-dev',
        'rpm': 'pango-devel',
        'gentoo': 'x11-libs/pango',
        'arch': 'pango',
    }
)
