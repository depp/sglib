# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .target import BaseTarget, ExternalTarget
from .external import ExternalBuildParameters
from ..error import ConfigError, UserError
from ..plist import ascii as plist
import os
import shutil
import random

# Map from file types to phases
PHASES = {k: v for v, kk in {
    'PBXFrameworksBuildPhase': [
        'file.xib',
        'image.icns',
    ],
    'PBXSourcesBuildPhase': [
        'sourcecode.c.c',
        'sourcecode.cpp.cpp',
        'sourcecode.c.objc',
    ],
    'PBXFrameworksBuildPhase': [
        'wrapper.framework',
    ],
}.items() for k in kk}

TYPE_MAP = {
    'c': 'sourcecode.c.c',
    'c++': 'sourcecode.cpp.cpp',
    'h': 'sourcecode.c.h',
    'h++': 'sourcecode.cpp.h',
    'objc': 'sourcecode.c.objc',
}

EXT_MAP = {
    '.xib': 'file.xib',
}
EXT_MAP.update({ext: 'image' + ext for ext in
                '.png .jpeg .pdf .git .bmp .pict .ico .icns .tiff'.split()})

class XcodeExternalTarget(ExternalTarget):
    __slots__ = []
    def build(self):
        params = ExternalBuildParameters(
            builddir=self.builddir, destdir=self.destdir)
        self.target.build(params)

class XcodeTarget(BaseTarget):
    """Object for creating an Xcode project."""
    __slots__ = [
        # The project name
        '_name',
        # Map from object IDs to objects.
        '_objects',
        # Map from id(obj) to object id.
        '_objectids',
        # Counter for generating unique object IDs.
        '_idcounter',
        # Map from paths to directory objects.
        '_path_directory',
        # Map from paths to PBXFileReference IDs.
        '_path_file',
        # Map from frameworks to PBXFileReference IDs.
        '_frameworks',

        # Archive containing the project.
        '_project_archive',
        # The project object.
        '_project',
        # The group containing products.
        '_products_group',
        # The group containing referenced frameworks.
        '_frameworks_group',
    ]

    def __init__(self, name, script, config, env):
        super(XcodeTarget, self).__init__(name, script, config, env)
        self._name = name
        self._objects = {}
        self._objectids = {}
        self._idcounter = random.getrandbits(96)
        self._path_directory = {}
        self._path_file = {}
        self._frameworks = {}

        self._products_group = {
            'isa': 'PBXGroup',
            'name': 'Products',
            'sourceTree': '<group>',
            'children': [],
        }
        products_group = self._add(self._products_group)

        self._frameworks_group = {
            'isa': 'PBXGroup',
            'name': 'Frameworks',
            'path': '/System/Library/Frameworks',
            'sourceTree': '<absolute>',
            'children': [],
        }
        frameworks_group = self._add(self._frameworks_group)

        self._project = {
            'isa': 'PBXProject',
            'compatibilityVersion': 'Xcode 3.2',
            'hasScannedForEncodings': 0,
            'projectDirPath': '',
            'projectRoot': '',
            'mainGroup': self._add({
                'isa': 'PBXGroup',
                'children': [
                    products_group,
                    frameworks_group,
                ],
                'sourceTree': '<group>'
            }),
            'buildConfigurationList': self._add({
                'isa': 'XCConfigurationList',
                'defaultConfigurationName': 'Release',
                'buildConfigurations': [
                    self._add({
                        'isa': 'XCBuildConfiguration',
                        'name': configuration,
                        'buildSettings': env.base_vars[configuration],
                        'defaultConfigurationIsVisible': 0,
                    })
                    for configuration in env.configurations
                ]
            }),
            'productRefGroup': products_group,
            'targets': [],
        }
        project = self._add(self._project)

        self._project_archive = {
            'archiveVersion': 1,
            'classes': {},
            'objectVersion': 46,
            'objects': self._objects,
            'rootObject': project,
        }

    def finalize(self):
        super(XcodeTarget, self).finalize()
        for obj in self._objects.values():
            if obj['isa'] != 'PBXGroup':
                continue
            obj['children'].sort(key=self._get_name)

        project_name = self._name + '.xcodeproj'
        shutil.rmtree(project_name, ignore_errors=True)
        os.mkdir(project_name)
        project_path = os.path.join(project_name, 'project.pbxproj')
        with open(project_path, 'w') as fp:
            w = plist.Writer(fp)
            w.write_object(self._project_archive)

    def external_target(self, obj, name, dependencies=[]):
        """Create an external target, without adding it to the build."""
        libbuild = os.path.join(self.env.library_path, 'build')
        destdir = os.path.join(libbuild, 'products')
        builddir = os.path.join(libbuild, name)
        return XcodeExternalTarget(
            obj, name, dependencies, destdir, builddir)

    def add_application_bundle(self, name, module, info_plist):
        """Create an OS X application bundle target."""
        if module is None:
            return ''

        appfile = self._add({
            'isa': 'PBXFileReference',
            'path': name + '.app',
            'sourceTree': 'BUILT_PRODUCTS_DIR',
            'explicitFileType': 'wrapper.application',
            'includeInIndex': 0,
        })
        self._products_group['children'].append(appfile)

        varset = {
            # 'ASSETCATALOG_COMPILER_APPICON_NAME': icon name,
            'COMBINE_HIDPI_IMAGES': True,
            'INFOPLIST_FILE': info_plist,
            'LD_RUNPATH_SEARCH_PATHS':
                '$(inherited) @executable_path/../Frameworks',
            'PRODUCT_NAME': '$(TARGET_NAME)',
        }
        varset = self.env.schema.merge([varset] + module.varsets)
        xvarset = {k: v for k, v in varset.items() if not k.startswith('.')}

        phases = [
            'PBXResourcesBuildPhase',
            'PBXSourcesBuildPhase',
            'PBXFrameworksBuildPhase',
        ]
        target = self._add({
            'isa': 'PBXNativeTarget',
            'productType': 'com.apple.product-type.application',
            'name': name,
            'productName': name,
            'productReference': appfile,
            'buildConfigurationList': self._add({
                'isa': 'XCConfigurationList',
                'defaultConfigurationName': 'Release',
                'buildConfigurations': [
                    self._add({
                        'isa': 'XCBuildConfiguration',
                        'name': configuration,
                        'buildSettings': xvarset,
                        'defaultConfigurationIsVisible': 0,
                    })
                    for configuration in self.env.configurations
                ],
            }),
            'dependencies': [],
            'buildPhases': [
                self._add({
                    'isa': phase,
                    'buildActionMask': 0x7fffffff,
                    'runOnlyForDeploymentPostprocessing': 0,
                    'files': [],
                })
                for phase in phases
            ],
            'buildRules': [],
        })
        self._project['targets'].append(target)

        sources = []
        for source in module.sources:
            sources.append(self._add_source(source))
        for framework in set(varset.get('.FRAMEWORKS', ())):
            sources.append(self._add_framework(framework))
        self._add_target_sources(target, sources)

        return ''

    def external_build_path(self, obj):
        """Get the build path for an external target."""
        return os.path.join(self.env.library_path, 'build', 'out')

    def _add(self, obj):
        """Add an object to the project, returning the object ID."""
        try:
            return self._objectids[id(obj)]
        except KeyError:
            pass
        if not isinstance(obj, dict):
            raise TypeError('object must be dict')
        if 'isa' not in obj:
            raise ValueError('missing ISA')
        objid = self._idcounter
        self._idcounter = (objid + 1) & ((1 << 96) - 1)
        objid = '{:24X}'.format(objid)
        self._objects[objid] = obj
        self._objectids[id(obj)] = objid
        return objid

    def _get_name(self, objid):
        """Get the name for a PBXGroup or PBXFileReference."""
        obj = self._objects[objid]
        isa = obj['isa']
        if isa == 'PBXGroup':
            return obj.get('name') or obj.get('path') or ''
        elif isa == 'PBXFileReference':
            return obj.get('path') or ''
        return ''

    def _add_directory(self, path):
        """Add a directory to the project, if necessary."""
        try:
            return self._path_directory[path]
        except KeyError:
            pass
        if not path:
            parent = self._objects[self._project['mainGroup']]
            obj = {
                'name': 'Sources',
                'sourceTree': '<group>',
            }
        else:
            dirname, basename = os.path.split(path)
            if basename == '..':
                raise ConfigError('path outside root')
            parent = self._add_directory(dirname)
            obj = {
                'path': basename,
                'sourceTree': '<group>'
            }
        obj.update({
            'isa': 'PBXGroup',
            'children': [],
        })
        objid = self._add(obj)
        parent['children'].append(objid)
        self._path_directory[path] = obj
        return obj

    def _add_source(self, source):
        """Add a source file to the project, if necessary.

        Returns the ID of the PBXFileReference.
        """
        try:
            return self._path_file[source.path]
        except KeyError:
            pass
        dirname, basename = os.path.split(source.path)
        parent = self._add_directory(dirname)
        try:
            ftype = TYPE_MAP[source.sourcetype]
        except KeyError:
            try:
                ftype = EXT_MAP[os.path.splitext(source.path)[1]]
            except KeyError:
                raise ConfigError(
                    'file has unknown type: {}'.format(source.path))
        obj = {
            'isa': 'PBXFileReference',
            'sourceTree': '<group>',
            'lastKnownFileType': ftype,
            'path': basename,
        }
        if ftype.startswith('sourcecode.') or ftype.startswith('text.'):
            obj['fileEncoding'] = 4
        objid = self._add(obj)
        parent['children'].append(objid)
        self._path_file[source.path] = objid
        return objid

    def _add_framework(self, name):
        """Add the framework with the given name, if necessary.

        Returns the ID of the PBXFileReference.
        """
        try:
            return self._frameworks[name]
        except KeyError:
            pass
        fwname = name + '.framework'
        try:
            fwpath = self.env.find_framework(name)
        except ConfigError:
            fwpath = fwname
        obj = {
            'isa': 'PBXFileReference',
            'lastKnownFileType': 'wrapper.framework',
        }
        if os.path.dirname(fwpath) == '/System/Library/Frameworks':
            obj.update({
                'path': fwname,
                'sourceTree': '<group>',
            })
        else:
            obj.update({
                'name': fwname,
                'path': fwpath,
                'sourceTree': '<absolute>',
            })
        objid = self._add(obj)
        self._frameworks_group['children'].append(objid)
        self._frameworks[name] = objid
        return objid

    def _add_target_sources(self, targetid, sourceids):
        """Add sources to a target's build phases."""
        phases = [self._objects[phaseid]
                  for phaseid in self._objects[targetid]['buildPhases']]
        phases = {obj['isa']: obj for obj in phases}
        for sourceid in sourceids:
            source = self._objects[sourceid]
            if source['isa'] != 'PBXFileReference':
                raise ValueError('source is not a file: {}'
                                 .format(source['isa']))
            ftype = (source.get('lastKnownFileType') or
                     source.get('explicitFileType'))
            if not ftype:
                raise ValueError('no file type for source')
            try:
                phaseisa = PHASES[ftype]
            except KeyError:
                continue
            try:
                phase = phases[phaseisa]
            except KeyError:
                raise ValueError('missing phase: {}'.format(phaseisa))
            phase['files'].append(self._add({
                'isa': 'PBXBuildFile',
                'fileRef': sourceid,
            }))
