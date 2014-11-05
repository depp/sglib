# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage
from d3build.error import ConfigError
import os

def pkg_config(build):
    return None, build.target.module().add_pkg_config('gl')

def framework(build):
    return None, build.target.module().add_framework(name='OpenGL')

def windows(build):
    build.check_platform('windows')
    return None, build.target.module().add_library('opengl32.lib')

module = ExternalPackage(
    [framework, pkg_config, windows],
    name='OpenGL',
    packages={
        'deb': 'libgl-dev',
        'rpm': 'mesa-libGL-devel',
        'gentoo': 'virtual/opengl',
        'arch': 'libgl',
    }
)
