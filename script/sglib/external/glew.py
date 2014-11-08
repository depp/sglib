# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage
from d3build.error import ConfigError
from d3build.source import SourceList
from ..libs import  binary_lib
import os

def pkg_config(build):
    flags = build.pkg_config('glew')
    return None, build.target.module().add_flags(flags)

def find_package(build):
    return build.find_package('^glew(:?-[0-9.]+)?$')

def bundled_bin(build):
    path, mod = binary_lib(build, find_package(build), ['glew32s.lib'])
    mod.add_define('GLEW_STATIC')
    return path, mod

def bundled_src(build):
    path = find_package(build)
    src = SourceList(path=path)
    src.add('src/glew.c', tags=['external'])
    src.add(path='include/GL', tags=['external'], sources='''
    glew.h
    glxew.h
    wglew.h
    ''')
    mod = (build.target.module()
        .add_sources(src.sources, {})
        .add_define('GLEW_STATIC')
        .add_header_path(os.path.join(path, 'include'), system=True))
    # http://stackoverflow.com/questions/22759943/unable-to-build-against-glew-on-os-x-10-9-mavericks
    if build.config.target == 'xcode':
        mod.add_variables({'CLANG_ENABLE_MODULES': 'NO'})
    return path, mod

module = ExternalPackage(
    [pkg_config, bundled_bin, bundled_src],
    name='The OpenGL Extension Wrangler Library',
    packages={
        'deb': 'libglew-dev',
        'rpm': 'glew-devel',
        'gentoo': 'media-libs/glew',
        'arch': 'glew',
    }
)
