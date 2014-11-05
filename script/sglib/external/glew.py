# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage
from d3build.error import ConfigError
from d3build.source import SourceList
import os

def pkg_config(build):
    flags = build.pkg_config('glew')
    return None, build.target.module().add_flags(flags)

def bundled(build):
    path = build.find_package('^glew(:?-[0-9.]+)?$')
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
        .add_define('USE_BUNDLED_GLEW')
        .add_header_path(os.path.join(path, 'include'), system=True))
    # http://stackoverflow.com/questions/22759943/unable-to-build-against-glew-on-os-x-10-9-mavericks
    if build.config.target == 'xcode':
        mod.add_variables({'CLANG_ENABLE_MODULES': 'NO'})
    return path, mod

module = ExternalPackage(
    [pkg_config, bundled],
    name='The OpenGL Extension Wrangler Library',
    packages={
        'deb': 'libglew-dev',
        'rpm': 'glew-devel',
        'gentoo': 'media-libs/glew',
        'arch': 'glew',
    }
)
