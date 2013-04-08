import build.config
import build.data as data
from build.path import Href
import os
import re

REQS = {
    'sglib': ['sglib'],
    'sglib++':  ['sglib++'],
}

APPS = {
    'cocoa': 'Cocoa',
    'gtk': 'Gtk',
    'sdl': 'SDL',
    'windows': 'Windows',
}

@data.template('sglib', 'sglib++')
def template_sglib(module, buildinfo, proj):
    moddir = proj.path(os.path.join(os.path.dirname(__file__), 'module'))

    src = data.Module(proj.gen_name(), 'source')
    src.group = module.group
    src.group.requirements.extend(
        data.Requirement(Href(moddir.join(req), None), False)
        for req in REQS[module.type])
    src.modules = module.modules
    yield src

    target_os = proj.environ['os']

    info = data.InfoDict()
    info.update(buildinfo)
    info.update(module.info)

    name = module.info.get_string('name', None)
    if name is None:
        name = buildinfo.get_string('name', None)
    if name is None:
        raise Exception('cannot determine application name')

    filename = module.info.get_string('filename', name)

    cvars = {k[5:]: v for k, v in info.items() if k.startswith('cvar.')}

    apps = []
    for app in APPS:
        if proj.environ['flag']['app-' + app] != 'yes':
            continue
        amod = data.Module(proj.gen_name(), 'executable')
        apps.append((app, amod))
        amod.group.requirements = [
            data.Requirement(x, False) for x in
            [src.name, Href(moddir.join('sglib-' + app), None)]]
        amod.info['name'] = ('{} ({})'.format(name, APPS[app]),)

    if target_os == 'linux':
        filename = re.sub(r'[^-A-Za-z0-9]+', '_', filename).strip('_')
        if not filename:
            raise Exception('default filename is empty')
        args = {'default-args.{}'.format(n+1):
                ('-d' + k + '=',) + v
                for n, (k, v) in enumerate(cvars.items())}
        for app, amod in apps:
            amod.info.update(args)
            amod.info['filename'] = ['{}-{}'.format(filename, app)]
    else:
        raise Exception('unsupported os: {}'.format(target_os))

    for app, amod in apps:
        yield amod

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
