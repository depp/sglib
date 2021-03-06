# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage
from d3build.error import ConfigError
from ..libs import binary_lib
import os

def pkg_config(build, version):
    flags = build.sdl_config(version)
    return None, build.target.module().add_flags(flags)

SDL_NAME = {1: 'SDL', 2: 'SDL2'}

def framework(build, version):
    name = SDL_NAME[version]
    path = build.find_framework(name)
    return None, (build.target.module()
        .add_header_path(os.path.join(path, 'Headers'))
        .add_framework(path=path))

def bundled_bin(build, version):
    build.check_platform('windows')
    name = SDL_NAME[version]
    path = build.find_package(
        '^{}(-[0-9.]+)$'.format(name),
        varname=name + '_PATH')
    libs = ['{}.lib'.format(name), '{}main.lib'.format(name)]
    return binary_lib(build, path, libs)

version_1 = ExternalPackage(
    [pkg_config, framework, bundled_bin],
    args=(1,),
    name='LibSDL',
    packages={
        'deb': 'libsdl1.2-dev',
        'rpm': 'SDL-devel',
        'gentoo': 'media-libs/libsdl',
        'arch': 'sdl',
    }
)

version_2 = ExternalPackage(
    [pkg_config, framework, bundled_bin],
    args=(2,),
    name='LibSDL 2',
    packages={
        'deb': 'libsdl2-dev',
        'rpm': 'SDL2-devel',
        'gentoo': 'media-libs/libsdl2',
        'arch': 'sdl2',
    }
)
