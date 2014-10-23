# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule
from d3build.error import try_config
from d3build.source import SourceList
import os

def pkg_config(build):
    return None, [], {'public': [build.env.pkg_config('ogg')]}

# Note: should set:
# -ffast-math
# -falign-loops=16

def bundled(build):
    env = build.env
    path = env.find_library('^libogg-[0-9.]+$')
    src = SourceList(path=path, sources='''
    src/bitwise.c
    src/framing.c
    ogg/ogg.h
    ogg/os_types.h
    ''')
    varsets = [
        env.header_path(os.path.join(path, 'include'), system=True),
    ]
    return path, src.sources, {'public': varsets}

def _configure(build):
    return try_config([build], pkg_config, bundled)

module = ExternalModule(
    name='Ogg bitstream library',
    configure=_configure,
    packages={
        'deb': 'libogg-dev',
        'rpm': 'libogg-devel',
        'gentoo': 'media-libs/libogg',
        'arch': 'libogg',
    }
)
