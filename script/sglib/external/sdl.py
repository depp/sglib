# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule
from d3build.error import ConfigError, try_config
import os

def pkg_config(build, version):
    return None, [], {'public': [build.env.sdl_config(version)]}

SDL_NAME = {1: 'SDL', 2: 'SDL2'}

def framework(build, version):
    env = build.env
    name = SDL_NAME[version]
    path = env.find_framework(name)
    varsets = [
        env.header_path(os.path.join(path, 'Headers')),
        env.framework_path(os.path.dirname(path)),
        env.frameworks([name])]
    return None, [], {'public': [env.schema.merge(varsets)]}

def windows(build, version):
    env = build.env
    env.check_platform('windows')
    name = SDL_NAME[version]
    path = env.find_library(
        '^{}(-[0-9.]+)$'.format(name),
        varname=name + '_PATH')
    # FIXME: use per-platform path.
    varsets = [
        env.header_path(os.path.join(path, 'include')),
        env.library_path(os.path.join(path, 'lib', 'x86')),
        env.library('{}.lib'.format(name)),
        env.library('{}main.lib'.format(name)),
    ]
    return path, [], {'public': [env.schema.merge(varsets)]}

def configure(version):
    def configure_func(build):
        return try_config([build, version], pkg_config, framework, windows)
    return configure_func

version_1 = ExternalModule(
    name='LibSDL',
    configure=configure(1),
    packages={
        'deb': 'libsdl1.2-dev',
        'rpm': 'SDL-devel',
        'gentoo': 'media-libs/libsdl',
        'arch': 'sdl',
    }
)

version_2 = ExternalModule(
    name='LibSDL 2',
    configure=configure(2),
    packages={
        'deb': 'libsdl2-dev',
        'rpm': 'SDL2-devel',
        'gentoo': 'media-libs/libsdl2',
        'arch': 'sdl2',
    }
)
