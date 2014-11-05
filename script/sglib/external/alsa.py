# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage

def pkg_config(build):
    flags = build.pkg_config('alsa')
    return None, build.target.module().add_flags(flags)

module = ExternalPackage(
    [pkg_config],
    name='ALSA',
    packages={
        'deb': 'libasound2-dev',
        'rpm': 'alsa-lib-devel',
        'gentoo': 'media-libs/alsa-lib',
        'arch': 'alsa-lib',
    }
)
