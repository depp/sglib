from d3build.module import ExternalModule

def _configure(env):
    varset = env.pkg_config('gtkglext-1.0')
    varset.LIBS = tuple(flag for flag in varset.LIBS
                        if flag != '-Wl,--export-dynamic')
    return [], {'public': [varset]}

module = ExternalModule(
    name='Gtk+ OpenGL Extension',
    configure=_configure,
    packages={
        'deb': 'libgtkglext1-dev',
        'rpm': 'gtkglext-devel',
        'gentoo': 'x11-libs/gtkglext',
        'arch': 'gtkglext',
    }
)
