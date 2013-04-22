import build.config
import build.project.data as data
from build.path import Href, splitext
from build.error import ConfigError
import os
import re
import uuid

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

def gen_info_plist(info, main_nib, icon):
    version = info.get_string('version', '0.0')

    if 'copyright' in info:
        getinfo = '{}, {}'.format(version, info.get_string('copyright'))
    else:
        getinfo = version

    plist = {
        'CFBundleDevelopmentRegion': 'English',
        'CFBundleExecutable': '${EXECUTABLE_NAME}',
        'CFBundleGetInfoString': getinfo,
        # CFBundleName
        'CFBundleIconFile': icon,
        'CFBundleIdentifier': info.get_string('identifier'),
        'CFBundleInfoDictionaryVersion': '6.0',
        'CFBundlePackageType': 'APPL',
        'CFBundleShortVersionString': version,
        'CFBundleSignature': '????',
        'CFBundleVersion': version,
        'LSApplicationCategoryType':
            info.get_string('apple-category', 'public.app-category.games'),
        # LSArchicecturePriority
        # LSFileQuarantineEnabled
        'LSMinimumSystemVersion': '10.5.0',
        'NSMainNibFile': main_nib,
        'NSPrincipalClass': 'GApplication',
        # NSSupportsAutomaticTermination
        # NSSupportsSuddenTermination
    }
    import build.plist.xml as xml
    data = xml.dump({k:v for k, v in plist.items() if v is not None})
    return data.decode('UTF-8')

@data.template('sglib', 'sglib++')
def template_sglib(module, buildinfo, cfg, proj):
    sgroot = cfg.path(os.path.dirname(os.path.dirname(__file__)))
    moddir = sgroot.join('build/module')
    rsrcdir = sgroot.join('build/resource')

    src = data.Module(proj.gen_name(), 'source')
    src.group = module.group
    src.group.requirements.extend(
        data.Requirement(Href(moddir.join(req), None), False)
        for req in REQS[module.type])
    src.modules = module.modules
    yield src

    target_os = cfg.target.os

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
    extras = []
    for app in APPS:
        if cfg.flags['app-' + app] != 'yes':
            continue
        amod = data.Module(None, 'executable')
        apps.append((app, amod))
        amod.group.requirements = [
            data.Requirement(x, False) for x in
            [src.name, Href(moddir.join1('sglib-' + app), None)]]
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
    elif target_os == 'osx':
        # Find icon file
        icon = info.get_path('icon.osx', info.get_path('icon', None))
        if icon is not None:
            icon_name = icon.basename()
            icon_name, icon_ext = splitext(icon_name)
            if not icon_name or icon_ext != '.icns':
                raise ConfigError('invalid icon for OS X: {}'.format(icon))
            src.group.sources.append(data.Source(icon, 'resource'))
        else:
            icon_name = None

        # Generate property list
        plistpath = rsrcdir.join1(filename, '.plist')
        plist = data.Module(None, 'literal-file')
        plist.info['target'] = [plistpath]
        plist.info['contents'] = [gen_info_plist(info, filename, icon_name)]
        yield plist

        # Generate XIB file
        xibpath = rsrcdir.join1(filename, '.xib')
        xib = data.Module(None, 'template-file')
        xib.info['target'] = [xibpath]
        xib.info['source'] = [sgroot.join('sg/osx/MainMenu.xib')]
        xib.info['var.EXE_NAME'] = [filename]
        yield xib
        src.group.sources.append(data.Source(xibpath, 'xib'))

        # Generate application module
        args = []
        for k, v in cvars.items():
            args.extend((('-' + k,), v))
        args = {'default-args.{}'.format(n+1): v for n, v in enumerate(args)}
        for app, amod in apps:
            amod.info.update(args)
            amod.info['info-plist'] = [plistpath]
            amod.type = 'application-bundle'
            if app != 'cocoa':
                amod.info['filename'] = [
                    '{} ({})'.format(filename, APPS[app])]
            else:
                amod.info['filename'] = [filename]
    elif target_os == 'windows':
        args = {'default-args.{}'.format(n+1):
                ('/D' + k + '=',) + v
                for n, (k, v) in enumerate(cvars.items())}

        icon = info.get_path('icon.windows', None)
        if icon is not None:
            rcpath = rsrcdir.join1(filename, '.rc')
            rcmod = data.Module(None, 'literal-file')
            rcmod.info['target'] = [rcpath]
            rcmod.info['contents'] = [
                '1 ICON "{}"\n'.format(cfg.target_path(icon))]
            yield rcmod
            src.group.sources.append(data.Source(rcpath, 'rc'))

        for app, amod in apps:
            if (len(apps) == 1 or app == 'windows') and 'uuid' in module.info:
                amod.info['uuid'] = module.info['uuid']
            else:
                amod.info['uuid'] = [str(uuid.uuid4())]
            amod.info.update(args)
            if app != 'windows':
                amod.info['filename'] = ['{} {}'.format(filename, APPS[app])]
            else:
                amod.info['filename'] = [filename]
    else:
        raise Exception('unsupported os: {}'.format(target_os))

    for app, amod in apps:
        yield amod

cfg = build.config.ConfigTool()
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
    'ogg', 'enable Ogg decoding')
cfg.add_enable(
    'vorbis', 'enable Vorbis decoding (requires Ogg)')
cfg.add_enable(
    'opus', 'enable Opus decoding (requires Ogg)')

cfg.add_enable(
    'app-cocoa', 'enable Cocoa frontend')
cfg.add_enable(
    'app-gtk', 'enable Gtk frontend')
cfg.add_enable(
    'app-sdl', 'enable LibSDL frontend')
cfg.add_enable(
    'app-windows', 'enable Windows frontend')

cfg.add_defaults({
    None: {
        'ogg': 'yes',
        'vorbis': 'yes',
        'opus': 'yes',
    },
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

cfg.add_path(
    os.path.join(os.path.dirname(os.path.dirname(__file__)),
                 'build', 'module'))

cfg.run()
