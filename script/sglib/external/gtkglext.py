# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage

def pkg_config(build):
    flags = build.pkg_config('gtkglext-1.0')
    flags['LIBS'] = [flag for flag in flags['LIBS']
                     if flag != '-Wl,--export-dynamic']
    return None, build.target.module().add_flags(flags)

module = ExternalPackage(
    [pkg_config],
    name='Gtk+ OpenGL Extension',
    packages={
        'deb': 'libgtkglext1-dev',
        'rpm': 'gtkglext-devel',
        'gentoo': 'x11-libs/gtkglext',
        'arch': 'gtkglext',
    }
)
