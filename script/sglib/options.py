# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.options import EnableFlag

flags = [
    EnableFlag('frontend', 'choose a front end', {
        'cocoa': 'use Cocoa front end',
        'gtk': 'use Gtk frontend',
        'sdl': 'use LibSDL frontend',
        'windows': 'use Windows frontend',
    }, mandatory=True),
    EnableFlag('audio', 'enable audio output', {
        'alsa': 'use ALSA',
        'coreaudio': 'use Core Audio',
        'directsound': 'use DirectSound',
        'sdl': 'use LibSDL',
    }, require=['audio']),
    EnableFlag('vorbis', 'enable Vorbis decoding'),
    EnableFlag('opus', 'enable Opus decoding'),
    EnableFlag('jpeg', 'enable JPEG decoding', {
        'libjpeg': 'use LibJPEG',
        'coregraphics': 'use Core Graphics',
        'wincodec': 'use WinCodec',
    }, require=['jpeg']),
    EnableFlag('png', 'enable PNG decoding', {
        'libpng': 'use LibPNG',
        'coregraphics': 'use Core Graphics',
        'wincodec': 'use WinCodec',
    }, require=['png']),
    EnableFlag('type', 'enable type rendering', {
        'pango': 'use Pango',
        'coretext': 'use Core Text',
        'uniscribe': 'use Uniscribe',
    }),
    EnableFlag('video-recording', 'enable video recording'),
]

defaults = {
    None: {
        'vorbis': True,
        'opus': True,
        'video-recording': False,
    },
    'linux': {
        'audio': 'sdl',
        'jpeg': 'libjpeg',
        'png': 'libpng',
        'type': 'pango',
        'frontend': 'sdl',
    },
    'osx': {
        'audio': 'coreaudio',
        'jpeg': 'coregraphics',
        'png': 'coregraphics',
        'type': 'coretext',
        'frontend': 'cocoa',
    },
    'windows': {
        'audio': 'directsound',
        'jpeg': 'wincodec',
        'png': 'wincodec',
        'type': 'uniscribe',
        'frontend': 'windows',
    },
}
