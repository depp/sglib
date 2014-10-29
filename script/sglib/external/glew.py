# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule
from d3build.error import ConfigError, try_config
from d3build.source import SourceList
import os

def pkg_config(build):
    return None, [], {'public': [build.env.pkg_config('glew')]}

def bundled(build):
    env = build.env
    path = env.find_library('^glew(:?-[0-9.]+)?$')
    src = SourceList(path=path)
    src.add('src/glew.c')
    src.add(path='include/GL', sources='''
    glew.h
    glxew.h
    wglew.h
    ''')
    varsets = [
        env.define('GLEW_STATIC'),
        env.define('USE_BUNDLED_GLEW'),
        env.header_path(os.path.join(path, 'include'), system=True),
    ]
    # http://stackoverflow.com/questions/22759943/unable-to-build-against-glew-on-os-x-10-9-mavericks
    if build.config.target == 'xcode':
        varsets.append({'CLANG_ENABLE_MODULES': 'NO'})
    return path, src.sources, {'public': [env.schema.merge(varsets)]}

def _configure(build):
    return try_config([build], pkg_config, bundled)

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
