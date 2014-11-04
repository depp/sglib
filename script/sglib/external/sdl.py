# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage
from d3build.error import ConfigError
import os

def pkg_config(build, version):
    return None, build.target.module().add_sdl_config(version)

SDL_NAME = {1: 'SDL', 2: 'SDL2'}

def framework(build, version):
    name = SDL_NAME[version]
    path = build.find_framework(name)
    return None, (build.target.module()
        .add_header_path(os.path.join(path, 'Headers'))
        .add_framework(path=path))

WIN_DIR = {'Win32': 'x86', 'x64': 'x64'}
def windows(build, version):
    build.check_platform('windows')
    name = SDL_NAME[version]
    path = build.find_package(
        '^{}(-[0-9.]+)$'.format(name),
        varname=name + '_PATH')
    # FIXME: use per-platform path.
    mod = build.target.module()
    mod.add_header_path(os.path.join(path, 'include'))
    libs = ['{}.lib'.format(name), '{}main.lib'.format(name)]
    for lib in libs:
        mod.add_library(lib)
    for platform in build.target.schema.platforms:
        if platform not in WIN_DIR:
            raise ConfigError('unknown platform: {}'.format(platform))
        ppath = os.path.join(path, 'lib', WIN_DIR[platform])
        for lib in libs:
            libpath = os.path.join(ppath, lib)
            if not os.path.exists(libpath):
                raise ConfigError('library does not exist: {}'
                                  .format(libpath))
    return None, mod

version_1 = ExternalPackage(
    [pkg_config, framework, windows],
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
    [pkg_config, framework, windows],
    args=(2,),
    name='LibSDL 2',
    packages={
        'deb': 'libsdl2-dev',
        'rpm': 'SDL2-devel',
        'gentoo': 'media-libs/libsdl2',
        'arch': 'sdl2',
    }
)
