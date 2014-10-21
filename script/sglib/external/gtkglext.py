# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule

def _configure(build):
    varset = build.env.pkg_config('gtkglext-1.0')
    varset['LIBS'] = tuple(flag for flag in varset['LIBS']
                           if flag != '-Wl,--export-dynamic')
    return None, [], {'public': [varset]}

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
