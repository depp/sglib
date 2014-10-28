# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.source import _base, _join, TagSourceFile
from d3build.module import Module
from .path import _path
import os

SIZES = [16, 32, 48, 64, 128, 256, 512, 1024]
_OSX_SIZES = [16, 32, 128, 256, 512]

class IconModule(Module):
    """A module consisting of an icon to include."""
    __slots__ = ['icon', 'sources']

    def __init__(self, *, icon, sources):
        self.icon = icon
        self.sources = sources

    def _get_configs(self, build):
        varsets = []
        if build.config.target == 'xcode':
            varsets.append({
                'ASSETCATALOG_COMPILER_APPICON_NAME': self.icon,
            })
        return [], {'private': varsets}

class Icon(object):
    """An icon, consisting of images in various formats and resolutions.

    For Windows, an ico file must be present, and should have square sizes 16,
    32, 48, and 256.  For OS X, PNG files are needed in square sizes from 16
    to 1024, covering each power of two.  It is okay if some sizes are
    missing.
    """
    __slots__ = ['ico', 'png']

    def __init__(self, *, base, path='.', sources):
        self.ico = None
        self.png = {}
        path = _base(base, path)
        for source in sources.splitlines():
            fields = source.split()
            if not fields:
                continue
            if len(fields) != 2:
                raise ValueError('must have two fields per line')
            fpath = _join(path, fields[1])
            if fields[0] == 'ico':
                self.ico = fpath
            else:
                try:
                    size = int(fields[0])
                except ValueError:
                    raise ValueError('invalid icon component: {!r}'
                                     .format(fields[0]))
                if size not in SIZES:
                    raise ValueError('invalid icon size: {}'.format(size))
                self.png[size] = fpath

    def module(self, build):
        """Get the icon module.

        Returns None if no icon is available for this platform.
        """
        if build.config.platform == 'osx':
            return self._module_osx(build)
        return None

    def _module_osx(self, build):
        from d3build.generatedsource.copyfile import CopyFile
        from d3build.generatedsource.simplefile import SimpleFile
        import json
        asset_path = _path('resources/Images.xcassets')
        path = os.path.join(asset_path, 'AppIcon.appiconset')
        images = []
        ssizes = set()
        for size in _OSX_SIZES:
            for scale in (1, 2):
                ssize = scale * size
                image = {
                    'size': '{0}x{0}'.format(size),
                    'idiom': 'mac',
                    'scale': '{}x'.format(scale),
                }
                images.append(image)
                if ssize not in self.png:
                    continue
                name = 'icon{}.png'.format(ssize)
                image['filename'] = name
                fpath = os.path.join(path, name)
                if ssize not in ssizes:
                    ssizes.add(ssize)
                    build.target.add_generated_source(
                        CopyFile(self.png[ssize], fpath))
        if not ssizes:
            return None
        info = {
            'images': images,
            'info': {
                'version': 1,
                'author': 'sglib'
            }
        }
        source = TagSourceFile(asset_path, (), None)
        build.target.add_generated_source(
            SimpleFile(os.path.join(path, 'Contents.json'),
                       json.dumps(info, indent=2)))
        return IconModule(icon='AppIcon', sources=[source])
