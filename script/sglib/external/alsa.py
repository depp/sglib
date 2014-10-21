# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule

def _configure(build):
    return [], {'public': [build.env.pkg_config('alsa')]}

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
