# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule
from d3build.error import ConfigError, try_config
import os

def pkg_config(build):
    return None, [], {'public': [build.env.pkg_config('gl')]}

def framework(build):
    return None, [], {'public': [build.env.frameworks(['OpenGL'])]}

def _configure(build):
    return try_config([build], framework, pkg_config)

module = ExternalModule(
    name='OpenGL',
    configure=_configure,
    packages={
        'deb': 'libgl-dev',
        'rpm': 'mesa-libGL-devel',
        'gentoo': 'virtual/opengl',
        'arch': 'libgl',
    }
)
