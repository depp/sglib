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
    }),
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

DEFAULTS = {
    'vorbis': True,
    'opus': True,
    'video-recording': False,
}

PLATFORM_DEFAULTS = {
    'linux': {
        'jpeg': 'libjpeg',
        'png': 'libpng',
        'type': 'pango',
        'frontend': 'sdl',
    },
    'osx': {
        'jpeg': 'coregraphics',
        'png': 'coregraphics',
        'type': 'coretext',
        'frontend': 'cocoa',
    },
    'windows': {
        'jpeg': 'wincodec',
        'png': 'wincodec',
        'type': 'uniscribe',
        'frontend': 'windows',
    },
}

AUDIO_DEFAULTS = {
    'linux': 'alsa',
    'osx': 'coreaudio',
    'windows': 'directsound',
}

def adjust_config(config, defaults):
    defaults_list = [
        defaults,
        PLATFORM_DEFAULTS[config.platform],
        DEFAULTS,
    ]
    for flags in defaults_list:
        if not flags:
            continue
        for flag, value in flags.items():
            if flag not in config.flags:
                raise ValueError('unknown flag: {!r}'.format(flag))
            if config.flags[flag] is None:
                config.flags[flag] = value
    if config.flags['audio'] is None:
        if config.flags['frontend'] == 'sdl':
            config.flags['audio'] = 'sdl'
        else:
            config.flags['audio'] = AUDIO_DEFAULTS[config.platform]
