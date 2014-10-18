from d3build.module import ExternalModule

def _configure(env):
    return [], {'public': [env.pkg_config('glew')]}

module = ExternalModule(
    name='The OpenGL Extension Wrangler Library',
    configure=_configure,
    packages={
        'deb': 'libglew-dev',
        'rpm': 'glew-devel',
        'gentoo': 'media-libs/glew',
        'arch': 'glew',
    }
)
