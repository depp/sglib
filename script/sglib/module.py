from d3build.module import SourceModule
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

def configure(env):
    from .external import (
        ogg, vorbis, opus,
        sdl, glew,
        alsa, libjpeg, libpng, pango, gtk, gtkglext,
    )

    tags = {tag: False for tag in TAGS}
    tags['public'] = [
        env.header_path(base=__file__, path='../../include'),
    ]

    if env.platform == 'linux':
        tags['posix'] = True
        tags['linux'] = True
    elif env.platform == 'osx':
        tags['posix'] = True
        tags['osx'] = True
    elif env.platform == 'windows':
        tags['windows'] = True

    if env.flags.frontend == 'cocoa':
        tags['frontend_cocoa'] = [env.module.frameworks(
            ['AppKit', 'Foundation', 'CoreVideo', 'OpenGL', 'Carbon'])]
    elif env.flags.frontend == 'gtk':
        tags['frontend_gtk'] = [gtkglext.module, gtk.module]
    elif env.flags.frontend == 'sdl':
        tags['frontend_sdl'] = [sdl.version_2]
    elif env.flags.frontend == 'windows':
        tags['frontend_windows'] = True
    else:
        raise Exception('invalid frontend flag: {!r}'
                        .format(env.flags.frontend))

    if env.flags.audio == 'alsa':
        tags['audio_alsa'] = [alsa.module]
    elif env.flags.audio == 'coreaudio':
        tags['audio_coreaudio'] = [env.module.frameworks(
            ['CoreAudio', 'AudioUnit'])]
    elif env.flags.audio == 'directsound':
        tags['audio_directsound'] = True
    elif env.flags.audio == 'sdl':
        tags['audio_sdl'] = [sdl.version_2]
    elif env.flags.audio == 'none':
        pass
    else:
        raise Exception('invalid audio flag: {!r}'
                        .format(env.flags.audio))

    enable_ogg = False
    if env.flags.vorbis:
        tags['vorbis'] = [vorbis.module]
        enable_ogg = True
    if env.flags.opus:
        tags['opus'] = [opus.module]
        enable_opus = True
    if enable_ogg:
        tags['ogg'] = [ogg.module]

    enable_coregraphics = False
    enable_wincodec = False

    if env.flags.jpeg == 'coregraphics':
        enable_coregraphics = True
    elif env.flags.jpeg == 'libjpeg':
        tags['image_libjpeg'] = [libjpeg.module]
    elif env.flags.jpeg == 'wincodec':
        enable_wincodec = True
    elif env.flags.jpeg == 'none':
        pass
    else:
        raise Exception('invalid jpeg flag: {!r}'
                        .format(env.flags.jpeg))

    if env.flags.png == 'coregraphics':
        enable_coregraphics = True
    elif env.flags.png == 'libpng':
        tags['image_libpng'] = [libpng.module]
    elif env.flags.png == 'wincodec':
        enable_wincodec = True
    elif env.flags.png == 'none':
        pass
    else:
        raise Exception('invalid png flag: {!r}'
                        .format(env.flags.png))

    if enable_coregraphics:
        tags['image_coregraphics'] = [env.module.frameworks(
            ['ApplicationServices'])]
    if enable_wincodec:
        tags['image_wincodec'] = True

    if env.flags.type == 'coretext':
        tags['type_coretext'] = [env.module.frameworks(
            ['ApplicationServices'])]
    elif env.flags.type == 'pango':
        tags['type_pango'] = [pango.module]
    elif env.flags.type == 'uniscribe':
        tags['type_uniscribe'] = True
    elif env.flags.type == 'none':
        pass
    else:
        raise Exception('invalid type flag: {!r}'
                        .format(env.flags.type))

    if env.flags.video_recording:
        tags['video_recording'] = True

    return tags

module = SourceModule(sources=source.src, configure=configure)
