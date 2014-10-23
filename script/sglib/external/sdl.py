# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule
from d3build.error import ConfigError, try_config
import os

def pkg_config(build, version):
    return None, [], {'public': [build.env.sdl_config(version)]}

def framework(build, version):
    env = build.env
    name = {1: 'SDL', 2: 'SDL2'}[version]
    path = env.find_framework(name)
    varsets = [
        env.header_path(os.path.join(path, 'Headers')),
        env.framework_path(os.path.dirname(path)),
        env.frameworks([name])]
    return None, [], {'public': [env.schema.merge(varsets)]}

def _configure(version):
    def configure(build):
        return try_config([build, version], pkg_config, framework)
    return configure

version_1 = ExternalModule(
    name='LibSDL',
    configure=_configure(1),
    packages={
        'deb': 'libsdl1.2-dev',
        'rpm': 'SDL-devel',
        'gentoo': 'media-libs/libsdl',
        'arch': 'sdl',
    }
)

version_2 = ExternalModule(
    name='LibSDL 2',
    configure=_configure(2),
    packages={
        'deb': 'libsdl2-dev',
        'rpm': 'SDL2-devel',
        'gentoo': 'media-libs/libsdl2',
        'arch': 'sdl2',
    }
)
