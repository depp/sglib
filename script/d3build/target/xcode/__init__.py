# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from . import obj
from ..target import BaseTarget
import os
import shutil

# Map from file types to phases
PHASES = {k: v for v, kk in {
    'resource': [
        'file.xib',
        'image.icns',
    ],
    'source': [
        'sourcecode.c.c',
        'sourcecode.cpp.cpp',
        'sourcecode.c.objc',
    ],
    'framework': [
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

class Target(object):
    """Object for creating an Xcode target."""
    __slots__ = ['_phases', '_target']

    def __init__(self, target):
        self._phases = {}
        self._target = target

    def add_phase(self, name, phase):
        if name:
            if name in self._phases:
                raise Exception('duplicate phase in target: {}'
                                .format(name))
            self._phases[name] = phase
        self._target.buildPhases.append(phase)

    def add_source(self, ref):
        ftype = ref.lastKnownFileType or ref.explicitFileType
        assert ftype
        try:
            phase = self._phases[PHASES[ftype]]
        except KeyError:
            # Ignore files with no phase and files where the phase
            # doesn't exist in this target
            return
        buildfile = obj.BuildFile(fileRef=ref)
        phase.files.append(buildfile)

    def add_executable(self, e):
        exes = self._target.executables
        if exes is None:
            self._target.executables = [e]
        else:
            exes.append(e)

class XcodeTarget(BaseTarget):
    """Object for creating an Xcode project."""
    __slots__ = [
        # The project name
        '_name',
        # The Xcode object version
        '_object_version',
        # Map from paths to Xcode groups
        '_directories',
        # Map from paths to Xcode source objects
        '_sources',
        # Map from framework names to Xcode framework objects
        '_frameworks',
        # The Xcode group object containing all framework references
        '_framework_group',
        # Xcode project object
        '_project',
    ]

    def __init__(self, name, script, config, env):
        super(XcodeTarget, self).__init__(name, script, config, env)

        self._name = name
        self._object_version = 46
        self._directories = {}
        self._sources = {}
        self._frameworks = {}
        products = obj.Group(name='Products', sourceTree='<group>')
        self._framework_group = obj.Group(
            name='Frameworks',
            path='/System/Library/Frameworks',
            sourceTree='<absolute>')
        self._project = obj.Project(
            compatibilityVersion='Xcode 3.2',
            hasScannedForEncodings=0,
            projectDirPath='',
            projectRoot='',
            mainGroup=obj.Group(
                children=[products, self._framework_group],
                sourceTree='<group>'),
            buildConfigurationList=obj.BuildConfigList(
                defaultConfigurationName='Release',
                buildConfigurations=[
                    obj.BuildConfiguration(
                        name=configuration,
                        buildSettings=env.base_vars[configuration])
                    for configuration in env.configurations]),
            productRefGroup=products,
        )

    def finalize(self):
        super(XcodeTarget, self).finalize()
        for x in self._project.allobjs():
            if isinstance(x, obj.Group):
                x.children.sort(key=lambda x: x.name or x.path)

        project_name = self._name + '.xcodeproj'
        shutil.rmtree(project_name, ignore_errors=True)
        os.mkdir(project_name)
        pp = os.path.join(project_name, 'project.pbxproj')
        up = os.path.join(project_name, 'default.pbxuser')
        with open(pp, 'w') as pf:
            with open(up, 'w') as uf:
                self._project.write(
                    pf, uf, objectVersion=self._object_version)

    def add_application_bundle(self, name, module, info_plist):
        """Create an OS X application bundle target."""
        if module is None:
            return ''

        appfile = obj.FileRef(
            path=name + '.app',
            sourceTree='BUILT_PRODUCTS_DIR',
            explicitFileType='wrapper.application',
            includeInIndex=0,
        )
        self._project.productRefGroup.children.append(appfile)

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

        target = obj.NativeTarget(
            productType='com.apple.product-type.application',
            name=name,
            productName=name,
            productReference=appfile,
            buildConfigurationList=obj.BuildConfigList(
                defaultConfigurationName='Release',
                buildConfigurations=[
                    obj.BuildConfiguration(
                        name=configuration,
                        buildSettings=xvarset)
                    for configuration in self.env.configurations]),
        )
        self._project.targets.append(target)

        tobj = Target(target)
        tobj.add_phase('resource', obj.ResourcesBuildPhase())
        tobj.add_phase('source', obj.SourcesBuildPhase())
        tobj.add_phase('framework', obj.FrameworksBuildPhase())
        for source in module.sources:
            tobj.add_source(self._add_source(source))
        for framework in varset.get('.FRAMEWORKS', ()):
            tobj.add_source(self._add_framework(framework))

        exe_args = []
        executable = obj.Executable(
            name=name,
            argumentStrings=exe_args,
            activeArgIndices=[True for arg in exe_args])
        tobj.add_executable(executable)

        if self._project.executables is None:
            self._project.executables = []
        self._project.executables.append(executable)

        return ''

    def _add_directory(self, path):
        """Add a directory to the project, if necessary."""
        try:
            return self._directories[path]
        except KeyError:
            pass
        if not path:
            srcgroup = obj.Group(name='Sources', sourceTree='<group>')
            self._project.mainGroup.children.append(srcgroup)
        else:
            dirname, basename = os.path.split(path)
            if basename == '..':
                raise ConfigError('path outside root')
            parent = self._add_directory(dirname)
            srcgroup = obj.Group(path=basename, sourceTree='<group>')
            parent.children.append(srcgroup)
        self._directories[path] = srcgroup
        return srcgroup

    def _add_source(self, source):
        """Add a source file to the project, if necessary."""
        try:
            return self._sources[source.path]
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
        fileEncoding = (4 if ftype.startswith('sourcecode.') or
                        ftype.startswith('text.') else None)
        src = obj.FileRef(
            path=basename,
            sourceTree='<group>',
            lastKnownFileType=ftype,
            fileEncoding=fileEncoding)
        parent.children.append(src)
        self._sources[source.path] = src
        return src

    def _add_framework(self, name):
        """Add the framework with the given name."""
        try:
            return self._frameworks[name]
        except KeyError:
            pass
        framework = obj.FileRef(
            path=name + '.framework',
            lastKnownFileType='wrapper.framework',
            sourceTree='<group>')
        self._framework_group.children.append(framework)
        self._frameworks[name] = framework
        return framework
