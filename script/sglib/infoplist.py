# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.target import GeneratedSource
from d3build.plist.xml import dump

class InfoPropertyList(GeneratedSource):
    __slots__ = [
        'target',
        'copyright', 'identifier', 'apple_category', 'main_nib',
    ]

    def __init__(self, target, app, main_nib):
        self.target = target
        self.copyright = app.copyright
        self.identifier = app.identifier
        self.apple_category = app.apple_category
        self.main_nib = main_nib

    @property
    def is_binary(self):
        return True

    def get_contents(self):
        version = '0.0'

        if self.copyright is not None:
            getinfo = '{}, {}'.format(version, self.copyright)
        else:
            getinfo = version

        plist = {
            'CFBundleDevelopmentRegion': 'English',
            'CFBundleExecutable': '${EXECUTABLE_NAME}',
            'CFBundleGetInfoString': getinfo,
            # CFBundleName
            'CFBundleIconFile': '',
            'CFBundleIdentifier': self.identifier,
            'CFBundleInfoDictionaryVersion': '6.0',
            'CFBundlePackageType': 'APPL',
            'CFBundleShortVersionString': version,
            'CFBundleSignature': '????',
            'CFBundleVersion': version,
            'LSApplicationCategoryType': self.apple_category,
            # LSArchicecturePriority
            # LSFileQuarantineEnabled
            'LSMinimumSystemVersion': '10.5.0',
            'NSMainNibFile': self.main_nib,
            'NSPrincipalClass': 'GApplication',
            # NSSupportsAutomaticTermination
            # NSSupportsSuddenTermination
        }
        return {k: v for k, v in plist.items() if v is not None}

    def write(self, fp):
        fp.write(dump(self.get_contents()))
