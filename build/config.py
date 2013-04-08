import build.config
import build.data as data
from build.path import Href
import os

REQS = {
    'sglib': ['sglib'],
    'sglib++':  ['sglib++'],
}

@data.template('sglib', 'sglib++')
def template_sglib(module, proj):
    moddir = proj.path(os.path.join(os.path.dirname(__file__), 'module'))

    src = data.Module(proj.gen_name(), 'source')
    src.group = module.group
    src.group.requirements.extend(
        data.Requirement(Href(moddir.join(req), None), False)
        for req in REQS[module.type])
    src.info = module.info
    src.modules = module.modules

    yield src

cfg = build.config.Config()
val = build.config.Value

cfg.add_enable(
    'audio', 'enable audio support',
    [val('alsa', 'ALSA'),
     val('coreaudio', 'Core Audio'),
     val('directsound', 'DirectSound')])
cfg.add_enable(
    'jpeg', 'enable JPEG image support',
    [val('libjpeg', 'LibJPEG'),
     val('coregraphics', 'Core Graphics'),
     val('wincodec', 'WinCodec')])
cfg.add_enable(
    'png', 'enable PNG image support',
    [val('libpng', 'LibPNG'),
     val('coregraphics', 'Core Graphics'),
     val('wincodec', 'WinCodec')])
cfg.add_enable(
    'type', 'enable type rendering support',
    [val('pango', 'Pango'),
     val('coretext', 'Core Text'),
     val('uniscribe', 'Uniscribe')])
cfg.add_enable(
    'video-recording', 'enable video recording support')

cfg.add_enable(
    'app-cocoa', 'enable Cocoa frontend')
cfg.add_enable(
    'app-gtk', 'enable Gtk frontend')
cfg.add_enable(
    'app-sdl', 'enable LibSDL frontend')
cfg.add_enable(
    'app-windows', 'enable Windows frontend')

cfg.add_defaults({
    'linux': {
        'audio': 'alsa',
        'jpeg': 'libjpeg',
        'png': 'libpng',
        'type': 'pango',
        'app-gtk': 'yes',
    },
    'osx': {
        'audio': 'coreaudio',
        'jpeg': 'coregraphics',
        'png': 'coregraphics',
        'type': 'coretext',
        'app-cocoa': 'yes',
    },
    'windows': {
        'audio': 'directsound',
        'jpeg': 'wincodec',
        'png': 'wincodec',
        'type': 'uniscribe',
        'app-windows': 'yes',
    },
})

cfg.run()
