# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import SourceModule
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
image_coregraphics
image_libjpeg
image_libpng
image_wincodec
linux
ogg
opus
osx
posix
type_coretext
type_pango
type_uniscribe
video_recording
vorbis
windows
'''.split()

def configure(build):
    from .external import (
        ogg, vorbis, opus,
        sdl, glew,
        alsa, libjpeg, libpng, pango, gtk, gtkglext,
    )
    platform = build.config.platform
    flags = build.config.flags

    tags = {tag: False for tag in TAGS}
    tags['public'] = [
        build.env.header_path(_base(__file__, '../../include')),
        glew.module,
    ]

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
        tags['frontend_cocoa'] = [build.env.frameworks(
            ['AppKit', 'Foundation', 'CoreVideo', 'OpenGL', 'Carbon'])]
    elif flags['frontend'] == 'gtk':
        tags['frontend_gtk'] = [gtkglext.module, gtk.module]
    elif flags['frontend'] == 'sdl':
        tags['frontend_sdl'] = [sdl.version_2]
    elif flags['frontend'] == 'windows':
        tags['frontend_windows'] = []
    else:
        raise ConfigError('invalid frontend flag: {!r}'
                          .format(flags['frontend']))

    if flags['audio'] == 'alsa':
        tags['audio_alsa'] = [alsa.module]
    elif flags['audio'] == 'coreaudio':
        tags['audio_coreaudio'] = [build.env.frameworks(
            ['CoreAudio', 'AudioUnit'])]
    elif flags['audio'] == 'directsound':
        tags['audio_directsound'] = []
    elif flags['audio'] == 'sdl':
        tags['audio_sdl'] = [sdl.version_2]
    elif flags['audio'] == 'none':
        pass
    else:
        raise ConfigError('invalid audio flag: {!r}'
                          .format(flags['audio']))

    enable_ogg = False
    if flags['vorbis']:
        tags['vorbis'] = [vorbis.module]
        enable_ogg = True
    if flags['opus']:
        tags['opus'] = [opus.module]
        enable_ogg = True
    if enable_ogg:
        tags['ogg'] = [ogg.module]

    enable_coregraphics = False
    enable_wincodec = False

    if flags['jpeg'] == 'coregraphics':
        enable_coregraphics = True
    elif flags['jpeg'] == 'libjpeg':
        tags['image_libjpeg'] = [libjpeg.module]
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
        tags['image_libpng'] = [libpng.module]
    elif flags['png'] == 'wincodec':
        enable_wincodec = True
    elif flags['png'] == 'none':
        pass
    else:
        raise ConfigError('invalid png flag: {!r}'
                          .format(flags['png']))

    if enable_coregraphics:
        tags['image_coregraphics'] = [build.env.frameworks(
            ['ApplicationServices'])]
    if enable_wincodec:
        tags['image_wincodec'] = []

    if flags['type'] == 'coretext':
        tags['type_coretext'] = [build.env.frameworks(
            ['ApplicationServices'])]
    elif flags['type'] == 'pango':
        tags['type_pango'] = [pango.module]
    elif flags['type'] == 'uniscribe':
        tags['type_uniscribe'] = []
    elif flags['type'] == 'none':
        pass
    else:
        raise ConfigError('invalid type flag: {!r}'
                          .format(flags['type']))

    if flags['video-recording']:
        tags['video_recording'] = []

    return tags

module = SourceModule(sources=source.src, configure=configure)
