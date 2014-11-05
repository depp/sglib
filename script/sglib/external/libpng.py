# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage

def pkg_config(build):
    flags = build.pkg_config('libpng12')
    return None, build.target.module().add_flags(flags)

module = ExternalPackage(
    [pkg_config],
    name='LibPNG',
    packages={
        'deb': 'libpng-dev',
        'rpm': 'libpng-devel',
        'gentoo': 'media-libs/libpng',
        'arch': 'libpng',
    }
)
