# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..target import BaseTarget
from ..error import ConfigError, UserError
from ..plist import ascii as plist
from .module import XcodeModule
from .schema import XcodeSchema
from . import base
from . import scheme
from xml.sax.saxutils import escape
import os
import shutil
import random
import io

# Map from file types to phases
PHASES = {k: v for v, kk in {
    'PBXResourcesBuildPhase': [
        'file.xib',
        'image.icns',
        'folder.assetcatalog',
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
    '.xcassets': 'folder.assetcatalog',
}
EXT_MAP.update({ext: 'image' + ext for ext in
                '.png .jpeg .pdf .git .bmp .pict .ico .icns .tiff'.split()})

class XcodeTarget(BaseTarget):
    """Object for creating an Xcode project."""
    __slots__ = [
        # The schema for build variables.
        'schema',
        # The base build variables, a module.
        'base',

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

        # Map from target name to scheme data.
        '_schemes',
    ]

    def __init__(self, build, name):
        super(XcodeTarget, self).__init__()
        self.schema = XcodeSchema()
        self.base = XcodeModule(self.schema)
        self.base.add_variables(base.BASE_CONFIG)
        self.base.add_variables(base.DEBUG_CONFIG, configs=['Debug'])
        self.base.add_variables(base.RELEASE_CONFIG, configs=['Release'])
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

        self._schemes = {}

    @property
    def run_srcroot(self):
        """The path root of the source tree, at runtime."""
        return '$(SRCROOT)'

    def module(self):
        return XcodeModule(self.schema)

    def finalize(self):
        self._project['buildConfigurationList'] = \
            self._add_build_configuration(self.base.variables())
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
        if self._schemes:
            schemedir = os.path.join(
                project_name, 'xcshareddata', 'xcschemes')
            os.makedirs(schemedir)
            for name, data in self._schemes.items():
                path = os.path.join(schemedir, name + '.xcscheme')
                with open(path, 'w') as fp:
                    fp.write(data)

    def add_application_bundle(self, *, name, module, info_plist,
                               arguments=[]):
        """Create an OS X application bundle target."""
        if not self._add_module(module):
            return

        appfile = self._add({
            'isa': 'PBXFileReference',
            'path': name + '.app',
            'sourceTree': 'BUILT_PRODUCTS_DIR',
            'explicitFileType': 'wrapper.application',
            'includeInIndex': 0,
        })
        self._products_group['children'].append(appfile)

        mod = self.module()
        mod.add_variables({
            # 'ASSETCATALOG_COMPILER_APPICON_NAME': icon name,
            'COMBINE_HIDPI_IMAGES': True,
            'INFOPLIST_FILE': info_plist,
            'LD_RUNPATH_SEARCH_PATHS':
                '$(inherited) @executable_path/../Frameworks',
            'PRODUCT_NAME': '$(TARGET_NAME)',
        })
        mod.add_module(module)

        phases = [
            'PBXSourcesBuildPhase',
            'PBXFrameworksBuildPhase',
            'PBXResourcesBuildPhase',
        ]
        target = self._add({
            'isa': 'PBXNativeTarget',
            'productType': 'com.apple.product-type.application',
            'name': name,
            'productName': name,
            'productReference': appfile,
            'buildConfigurationList':
                self._add_build_configuration(mod.variables()),
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
        for source in mod.sources:
            sources.append(self._add_source(source))
        self._add_target_sources(target, sources)

        if arguments:
            self._add_scheme(
                objid=target,
                name=name,
                filename=name + '.app',
                arguments=arguments)

        return ''

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
        objid = '{:024X}'.format(objid)
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

    def _add_build_configuration(self, variables):
        varset = variables.get_all()
        return self._add({
            'isa': 'XCConfigurationList',
            'defaultConfigurationName': 'Release',
            'buildConfigurations': [
                self._add({
                    'isa': 'XCBuildConfiguration',
                    'name': config,
                    'buildSettings': varset[config],
                    'defaultConfigurationIsVisible': 0,
                })
                for config in self.schema.configs
            ],
        })

    def _add_directory(self, path):
        """Add a directory to the project, if necessary."""
        if os.path.isabs(path):
            raise ConfigError('absolute path: {}'.format(path))
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
        if source.sourcetype == 'framework':
            return self._add_framework(source.path)
        try:
            return self._path_file[source.path]
        except KeyError:
            pass
        dirname, basename = os.path.split(source.path)
        parent = self._add_directory(dirname)
        if source.sourcetype is None:
            ftype = EXT_MAP[os.path.splitext(source.path)[1]]
        else:
            try:
                ftype = TYPE_MAP[source.sourcetype]
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

    def _add_framework(self, path):
        """Add the framework with the given path, if necessary.

        Returns the ID of the PBXFileReference.
        """
        try:
            return self._frameworks[path]
        except KeyError:
            pass
        name = os.path.basename(path)
        obj = {
            'isa': 'PBXFileReference',
            'lastKnownFileType': 'wrapper.framework',
        }
        if os.path.dirname(path) == '/System/Library/Frameworks':
            obj.update({
                'path': name,
                'sourceTree': '<group>',
            })
        else:
            obj.update({
                'name': name,
                'path': path,
                'sourceTree': '<absolute>',
            })
        objid = self._add(obj)
        self._frameworks_group['children'].append(objid)
        self._frameworks[path] = objid
        return objid

    def _add_target_sources(self, targetid, sourceids):
        """Add sources to a target's build phases."""
        phases = [self._objects[phaseid]
                  for phaseid in self._objects[targetid]['buildPhases']]
        phases = {obj['isa']: obj for obj in phases}
        phase_types = {k: set() for k in phases}
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
            phase_types[phaseisa].add(ftype)
            try:
                phase = phases[phaseisa]
            except KeyError:
                raise ValueError('missing phase: {}'.format(phaseisa))
            phase['files'].append(self._add({
                'isa': 'PBXBuildFile',
                'fileRef': sourceid,
            }))

        rtypes = phase_types['PBXResourcesBuildPhase']
        if 'folder.assetcatalog' in rtypes and len(rtypes) == 1:
            self._make_resource_dir_shell_script(targetid)

    def _make_resource_dir_shell_script(self, targetid):
        """Add a script which creates the Resources directory.

        This is a workaround for a bug where Xcode fails to create the
        directory if no resource files other than the asset catalog exist.
        """
        phases = self._objects[targetid]['buildPhases']
        for n, phase in enumerate(phases):
            if self._objects[phase]['isa'] == 'PBXResourcesBuildPhase':
                break
        else:
            assert False
        phases.insert(n, self._add({
            'isa': 'PBXShellScriptBuildPhase',
            'buildActionMask': 0x7fffffff,
            'runOnlyForDeploymentPostprocessing': 0,
            'files': [],
            'inputPaths': [],
            'outputPaths': [],
            'shellPath': '/bin/sh',
            'shellScript': MAKE_RESOURCE_DIR,
        }))

    def _add_scheme(self, *, objid, name, filename, arguments):
        argio = io.StringIO()
        for arg in arguments:
            argio.write(scheme.ARGUMENT.format(escape(arg)))
        self._schemes[name] = scheme.SCHEME.format(
            target_id=objid,
            target_filename=escape(filename),
            target_name=escape(name),
            project_name=escape(self._name),
            arguments=argio.getvalue(),
        )

MAKE_RESOURCE_DIR = '''
dirpath="${BUILT_PRODUCTS_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}"
test -d "${dirpath}" || mkdir "${dirpath}"
'''
