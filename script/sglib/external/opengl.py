# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule
from d3build.error import ConfigError, try_config
import os

def pkg_config(build):
    return None, [], {'public': [build.env.pkg_config('gl')]}

def framework(build):
    return None, [], {'public': [build.env.framework(['OpenGL'])]}

def windows(build):
    build.env.check_platform('windows')
    return None, [], {'public': [build.env.library('opengl32.lib')]}

def configure(build):
    return try_config([build], framework, pkg_config, windows)

module = ExternalModule(
    name='OpenGL',
    configure=configure,
    packages={
        'deb': 'libgl-dev',
        'rpm': 'mesa-libGL-devel',
        'gentoo': 'virtual/opengl',
        'arch': 'libgl',
    }
)
