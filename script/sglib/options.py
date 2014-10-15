from d3build.options import EnableFlag, parse_flags

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

d = parse_flags(flags)
print(d)
