# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule

def _configure(build):
    return [], {'public': [build.env.pkg_config('libpng12')]}

module = ExternalModule(
    name='LibPNG',
    configure=_configure,
    packages={
        'deb': 'libpng-dev',
        'rpm': 'libpng-devel',
        'gentoo': 'media-libs/libpng',
        'arch': 'libpng',
    }
)
