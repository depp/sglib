# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.generatedsource import GeneratedSource
from d3build.plist.xml import dump
from . import version

class InfoPropertyList(GeneratedSource):
    __slots__ = [
        'target',
        'copyright', 'identifier', 'apple_category', 'main_nib', 'icon',
        'app_path',
    ]

    def __init__(self, target, app, main_nib, icon, app_path):
        self.target = target
        self.copyright = app.copyright
        self.identifier = app.identifier
        self.apple_category = app.apple_category
        self.main_nib = main_nib
        self.icon = icon
        self.app_path = app_path

    @property
    def is_binary(self):
        return True

    def get_contents(self):
        ver = version.get_info('git', self.app_path)

        if self.copyright is not None:
            getinfo = '{}, {}'.format(ver.desc, self.copyright)
        else:
            getinfo = ver.desc

        plist = {
            'CFBundleDevelopmentRegion': 'English',
            'CFBundleExecutable': '$(EXECUTABLE_NAME)',
            'CFBundleGetInfoString': getinfo,
            # CFBundleName
            'CFBundleIconFile': self.icon,
            'CFBundleIdentifier': self.identifier,
            'CFBundleInfoDictionaryVersion': '6.0',
            'CFBundlePackageType': 'APPL',
            'CFBundleShortVersionString': ver.desc,
            'CFBundleSignature': '????',
            'CFBundleVersion': '{}.{}.{}'.format(*ver.number),
            'LSApplicationCategoryType': self.apple_category,
            # LSArchicecturePriority
            # LSFileQuarantineEnabled
            'LSMinimumSystemVersion': '10.7.0',
            'NSMainNibFile': self.main_nib,
            'NSPrincipalClass': 'GApplication',
            # NSSupportsAutomaticTermination
            # NSSupportsSuddenTermination
        }
        return {k: v for k, v in plist.items() if v is not None}

    def write(self, fp):
        fp.write(dump(self.get_contents()))
