# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule
from d3build.error import try_config
from d3build.source import SourceList
import os
from . import ogg

def pkg_config(build):
    return None, [], {'public': [build.env.pkg_config('vorbis')]}

# Note: should set:
# -ffast-math
# -falign-loops=16
# -D__MACOSX__

def bundled(build):
    env = build.env
    path = env.find_library('^libvorbis-[0-9.]+$')
    src = SourceList(path=path)
    src.add(path='lib', sources='''
    analysis.c
    bitrate.c
    block.c
    codebook.c
    envelope.c
    floor0.c
    floor1.c
    info.c
    lookup.c
    lpc.c
    lsp.c
    mapping0.c
    mdct.c
    psy.c
    registry.c
    res0.c
    sharedbook.c
    smallft.c
    synthesis.c
    window.c
    ''')
    return path, src.sources, {
        'public': [
            env.header_path(os.path.join(path, 'include'), system=True),
        ],
        'private': [
            env.header_path(os.path.join(path, 'lib')),
            ogg.module
        ],
    }

def _configure(build):
    return try_config([build], pkg_config, bundled)

module = ExternalModule(
    name='Vorbis codec library',
    configure=_configure,
    packages={
        'deb': 'libvorbis-dev',
        'rpm': 'libvorbis-devel',
        'gentoo': 'media-libs/libvorbis',
        'arch': 'libvorbis',
    }
)
