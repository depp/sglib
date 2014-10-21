# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule

def _configure(build):
    return [], {'public': [build.env.pkg_config('glew')]}

module = ExternalModule(
    name='The OpenGL Extension Wrangler Library',
    configure=_configure,
    packages={
        'deb': 'libglew-dev',
        'rpm': 'glew-devel',
        'gentoo': 'media-libs/glew',
        'arch': 'glew',
    }
)
