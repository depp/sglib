# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage

def pkg_config(build):
    flags = build.pkg_config('freetype2')
    return None, build.target.module().add_flags(flags)

module = ExternalPackage(
    [pkg_config],
    name='FreeType library',
    packages={
        'deb': 'libfreetype6-dev',
        'rpm': 'freetype-devel',
        'gentoo': 'media-libs/freetype',
        'arch': 'freetype2',
    }
)
