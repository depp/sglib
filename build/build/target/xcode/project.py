from . import obj
from build.path import Path
from build.object.literalfile import LiteralFile
from build.error import ConfigError
import posixpath

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

class SourceGroup(object):
    """Object for creating an Xcode group of source files."""
    __slots__ = ['_paths']

    def __init__(self, group):
        self._paths = {'': group}

    def get_dir(self, path):
        """Get the Xcode group for the given directory.

        Creates the group and any parent groups if necessary.
        The directory must be relative to the group root.
        """
        try:
            dirobj = self._paths[path]
        except KeyError:
            pass
        else:
            assert isinstance(dirobj, obj.Group)
            return dirobj
        assert path
        dirname, basename = posixpath.split(path)
        pardir = self.get_dir(dirname)
        dirobj = obj.Group(path=basename, sourceTree='<group>')
        pardir.children.append(dirobj)
        self._paths[path] = dirobj
        return dirobj

    def add_source(self, *, path, **kw):
        """Add a FileRef for a source file."""
        if path in self._paths:
            raise ConfigError('duplicate path: {}'.format(path))
        dirname, basename = posixpath.split(path)
        dirobj = self.get_dir(dirname)
        fileobj = obj.FileRef(path=basename, sourceTree='<group>', **kw)
        dirobj.children.append(fileobj)
        return fileobj

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

TARGET_TYPES = {
    'application_bundle',
}

class Project(object):
    """Object for creating an Xcode project."""
    __slots__ = ['_groups', '_sources', 'project',
                 '_frameworks', '_fwk_group', '_cfg',
                 '_objectVersion']

    def __init__(self, cfg):
        self._groups = {}
        self._sources = {}
        self._frameworks = {}
        self._cfg = cfg

        cfg_base = {
            'WARNING_CFLAGS': ['-Wall', '-Wextra'],
        }
        cfg_debug = {
            'GCC_OPTIMIZATION_LEVEL': 0,
        }
        cfg_release = {
            'ARCHS': ' '.join(self._cfg.target.archs),
            'GCC_OPTIMIZATION_LEVEL': 's',
        }
        if self._version <= (10, 6):
            cfg_base.update({
                'GCC_VERSION': '4.2',
            })
            cfg_debug.update({
                'ARCHS': '$(NATIVE_ARCH)',
            })
            compatibilityVersion = 'Xcode 2.4'
            self._objectVersion = 42
        else:
            cfg_base.update({
                'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
            })
            flags = (
                'CLANG_WARN_CONSTANT_CONVERSION',
                'CLANG_WARN_ENUM_CONVERSION',
                'CLANG_WARN_INT_CONVERSION',
                'CLANG_WARN__DUPLICATE_METHOD_MATCH',
                'GCC_WARN_ABOUT_RETURN_TYPE',
                'GCC_WARN_UNINITIALIZED_AUTOS',
                'GCC_WARN_UNUSED_VARIABLE',
            )
            for flag in flags:
                cfg_base[flag] = True
            cfg_debug.update({
                'ARCHS': '$(NATIVE_ARCH_ACTUAL)',
            })
            compatibilityVersion = 'Xcode 3.2'
            self._objectVersion = 46

        products = obj.Group(name='Products', sourceTree='<group>')
        self.project = obj.Project(
            compatibilityVersion=compatibilityVersion,
            hasScannedForEncodings=0,
            projectDirPath='',
            projectRoot='',
            mainGroup=obj.Group(children=[products], sourceTree='<group>'),
            buildConfigurationList=
                config_list(cfg_base, cfg_debug, cfg_release),
            productRefGroup=products,
        )

    @property
    def _version(self):
        return self._cfg.target.version

    def add_build(self, build, xcpath):
        self.add_sources(build)
        self.add_frameworks(build)
        for target in build.targets:
            target_type = target.target_type
            if target_type not in TARGET_TYPES:
                raise ConfigError(
                    'unsupported target type: {}'.format(target_type))
            func = getattr(self, 'add_' + target_type)
            func(build, target)
        for x in self.project.allobjs():
            if isinstance(x, obj.Group):
                x.children.sort(key=lambda x: x.name or x.path)

        import io
        pf = io.StringIO()
        uf = io.StringIO()
        self.project.write(pf, uf, objectVersion=self._objectVersion)
        build.add_generated_source(
            LiteralFile(target=xcpath.join('project.pbxproj'),
                        contents=pf.getvalue()))
        build.add_generated_source(
            LiteralFile(target=xcpath.join('default.pbxuser'),
                        contents=uf.getvalue()))

    def add_sources(self, build):
        srcgroup = obj.Group(name='Sources', sourceTree='<group>')
        self.project.mainGroup.children.append(srcgroup)
        if self._cfg.srcdir_target != '.':
            srcgroup.path = self._cfg.srcdir_target
        gobj = SourceGroup(srcgroup)
        for src in build.sources.values():
            try:
                ftype = TYPE_MAP[src.type]
            except KeyError:
                try:
                    ftype = EXT_MAP[src.path.splitext()[1]]
                except KeyError:
                    self._cfg.warn(
                        'file has unhandled type: {}'.format(src.path))
                    continue
            fileEncoding = (4 if ftype.startswith('sourcecode.') or
                                 ftype.startswith('text.') else None)
            fileobj = gobj.add_source(
                path=src.path.to_posix(),
                lastKnownFileType=ftype,
                fileEncoding=fileEncoding,
            )
            self._sources[src.path] = fileobj

    def add_frameworks(self, build):
        fwkgroup = obj.Group(
            name='Frameworks',
            path='/System/Library/Frameworks',
            sourceTree='<absolute>',
        )
        self.project.mainGroup.children.append(fwkgroup)
        for framework in build.modules.values():
            framework = framework.framework_name
            fileref = obj.FileRef(
                path=framework + '.framework',
                lastKnownFileType='wrapper.framework',
                sourceTree='<group>',
            )
            fwkgroup.children.append(fileref)
            self._frameworks[framework] = fileref

    def add_application_bundle(self, build, module):
        filename = module.filename
        source = build.sourcemodules[module.source]

        appfile = obj.FileRef(
            path=filename + '.app',
            sourceTree='BUILT_PRODUCTS_DIR',
            explicitFileType='wrapper.application',
            includeInIndex=0,
        )
        self.project.productRefGroup.children.append(appfile)
        cfg_base = {
            'ALWAYS_SEARCH_USER_PATHS': True,
            'INFOPLIST_FILE': self._cfg.target_path(module.info_plist),
            'INSTALL_PATH': '$(HOME)/Applications',
            'PRODUCT_NAME': filename,
            'USER_HEADER_SEARCH_PATHS':
                ['$(HEADER_SEARCH_PATHS)'] +
                [self._cfg.native_path(p) for p in source.header_paths],
        }
        if self._version <= (10, 6):
            cfg_base.update({
                'USE_HEADERMAP': False,
                'GCC_DYNAMIC_NO_PIC': False,
                # 'GCC_ENABLE_FIX_AND_CONTINUE': True,
                'GCC_MODEL_TUNING': 'G5',
                'PREBINDING': False,
            })
        else:
            cfg_base.update({
                'COMBINE_HIDPI_IMAGES': True,
            })
        cfg_debug = {}
        cfg_release = {}

        target = obj.NativeTarget(
            productType='com.apple.product-type.application',
            name=filename,
            productName=filename,
            productReference=appfile,
            buildConfigurationList=
                config_list(cfg_base, cfg_debug, cfg_release),
        )
        self.project.targets.append(target)

        tobj = Target(target)
        tobj.add_phase('resource', obj.ResourcesBuildPhase())
        tobj.add_phase('source', obj.SourcesBuildPhase())
        tobj.add_phase('framework', obj.FrameworksBuildPhase())
        for src in source.sources:
            tobj.add_source(self._sources[src.path])
        frameworks = {build.modules[dep].framework_name
                      for dep in source.deps}
        for framework in frameworks:
            tobj.add_source(self._frameworks[framework])

        executable = obj.Executable(
            name=filename,
            argumentStrings=[format_arg(self._cfg, arg)
                             for arg in module.args],
            activeArgIndices=[True for arg in module.args],
        )
        tobj.add_executable(executable)

        if self.project.executables is None:
            self.project.executables = []
        self.project.executables.append(executable)

def config_list(base, debug, release):
    cfg_debug = dict(base)
    cfg_debug.update(debug)
    cfg_release = dict(base)
    cfg_release.update(release)
    return obj.BuildConfigList(
        defaultConfigurationName='Release',
        buildConfigurations=[
            obj.BuildConfiguration(
                name='Debug', buildSettings=cfg_debug),
            obj.BuildConfiguration(
                name='Release', buildSettings=cfg_release),
        ],
    )

def format_arg(cfg, arg):
    parts = []
    for part in arg:
        if isinstance(part, str):
            parts.append(part)
        elif isinstance(part, Path):
            parts.append('$(PROJECT_DIR)')
            path = cfg.native_path(part)
            if path != '.':
                parts.append(part.path)
        else:
            raise TypeError('invalid argument')
    return ''.join(parts)
