# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule

def _configure(build):
    return None, [], {'public': [build.env.pkg_config('pangocairo')]}

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
