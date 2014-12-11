# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.error import ConfigError
from d3build.source import _base
from . import source

TAGS = '''
frontend_cocoa
frontend_gtk
frontend_sdl
frontend_windows
audio_alsa
audio_coreaudio
audio_directsound
audio_sdl
freetype
image_coregraphics
image_libjpeg
image_libpng
image_wincodec
linux
maybe_sdl
ogg
opus
osx
posix
video_recording
vorbis
windows
'''.split()

def get_tags(build):
    module = build.target.module
    from .external import (
        math,
        ogg, vorbis, opus,
        sdl, glew, opengl,
        alsa, libjpeg, libpng, pango, gtk, gtkglext,
        freetype,
    )
    platform = build.config.platform
    flags = build.config.flags

    tags = {tag: None for tag in TAGS}
    com = (module()
        .add_header_path(_base(__file__, '../../include'))
        .add_define('USE_BUNDLED_GLEW'))
    if platform == 'osx':
        com.add_variables({'MACOSX_DEPLOYMENT_TARGET': '10.7.0'})
    tags['public'] = [
        com,
        opengl.module(build),
        glew.module(build),
        math.module(build),
    ]
    tags['maybe_sdl'] = []

    if platform == 'linux':
        tags['posix'] = []
        tags['linux'] = []
    elif platform == 'osx':
        tags['posix'] = []
        tags['osx'] = []
    elif platform == 'windows':
        tags['windows'] = []
    else:
        raise ConfigError('unsupported platform')

    if flags['frontend'] == 'cocoa':
        mod = module()
        for name in 'AppKit Foundation CoreVideo OpenGL Carbon'.split():
            mod.add_framework(name=name)
        tags['frontend_cocoa'] = [mod]
    elif flags['frontend'] == 'gtk':
        tags['frontend_gtk'] = [gtkglext.module(build), gtk.module(build)]
    elif flags['frontend'] == 'sdl':
        tags['frontend_sdl'] = [sdl.version_2(build)]
        tags['maybe_sdl'].append(sdl.version_2(build))
    elif flags['frontend'] == 'windows':
        tags['frontend_windows'] = []
    else:
        raise ConfigError('invalid frontend flag: {!r}'
                          .format(flags['frontend']))

    if flags['audio'] == 'alsa':
        tags['audio_alsa'] = [alsa.module(build)]
    elif flags['audio'] == 'coreaudio':
        mod = module()
        for name in 'CoreAudio AudioUnit'.split():
            mod.add_framework(name=name)
        tags['audio_coreaudio'] = [mod]
    elif flags['audio'] == 'directsound':
        tags['audio_directsound'] = []
    elif flags['audio'] == 'sdl':
        tags['audio_sdl'] = [sdl.version_2(build)]
    elif flags['audio'] == 'none':
        pass
    else:
        raise ConfigError('invalid audio flag: {!r}'
                          .format(flags['audio']))

    enable_ogg = False
    if flags['vorbis']:
        tags['vorbis'] = [vorbis.module(build)]
        enable_ogg = True
    if flags['opus']:
        tags['opus'] = [opus.module(build)]
        enable_ogg = True
    if enable_ogg:
        tags['ogg'] = [ogg.module(build)]

    enable_coregraphics = False
    enable_wincodec = False

    if flags['jpeg'] == 'coregraphics':
        enable_coregraphics = True
    elif flags['jpeg'] == 'libjpeg':
        tags['image_libjpeg'] = [libjpeg.module(build)]
    elif flags['jpeg'] == 'wincodec':
        enable_wincodec = True
    elif flags['jpeg'] == 'none':
        pass
    else:
        raise ConfigError('invalid jpeg flag: {!r}'
                          .format(flags['jpeg']))

    if flags['png'] == 'coregraphics':
        enable_coregraphics = True
    elif flags['png'] == 'libpng':
        tags['image_libpng'] = [libpng.module(build)]
    elif flags['png'] == 'wincodec':
        enable_wincodec = True
    elif flags['png'] == 'none':
        pass
    else:
        raise ConfigError('invalid png flag: {!r}'
                          .format(flags['png']))

    if enable_coregraphics:
        tags['image_coregraphics'] = [
            module().add_framework(name='ApplicationServices')]
    if enable_wincodec:
        tags['image_wincodec'] = []

    if flags['freetype']:
        tags['freetype'] = [freetype.module(build)]

    if flags['video-recording']:
        tags['video_recording'] = []

    return tags

def module(build):
    return build.target.module().add_sources(source.src, get_tags(build))
