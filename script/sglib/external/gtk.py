# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule

def _configure(env):
    return [], {'public': [env.pkg_config('gtk+-2.0')]}

module = ExternalModule(
    name='Gtk+ 2.0',
    configure=_configure,
    packages={
        'deb': 'libgtk2.0-dev',
        'rpm': 'gtk2-devel',
        # FIXME: we need 2.0, not 1.0 or 3.0...
        'gentoo': 'x11-libs/gtk+',
        'arch': 'gtk2',
    }
)
