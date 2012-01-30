import buildtool.path
import buildtool.plist
import random
import posixpath
import sys
import cStringIO

TYPES = {
    'c': 'sourcecode.c.c',
    'cxx': 'sourcecode.cpp.cpp',
    'h': 'sourcecode.c.h',
    'hxx': 'sourcecode.cpp.h',
    'm': 'sourcecode.c.objc',
    'framework': 'wrapper.framework',
    'xib': 'file.xib',
    'plist': 'text.plist.xml',
}

RESOURCE_BUILD = set([
    'file.xib',
])

SOURCE_BUILD = set([
    'sourcecode.c.c',
    'sourcecode.cpp.cpp',
    'sourcecode.c.objc',
])

FRAMEWORK_BUILD = set([
    'wrapper.framework',
])

ISAS = [
    'PBXBuildFile',
    'PBXFileReference',
    'PBXFrameworksBuildPhase',
    'PBXGroup',
    'PBXNativeTarget',
    'PBXProject',
    'PBXResourcesBuildPhase',
    'PBXShellScriptBuildPhase',
    'PBXSourcesBuildPhase',
    'XCBuildConfiguration',
    'XCConfigurationList',
]

# UNSET = token('UNSET')

class optional(object):
    def __init__(self, what):
        self.what = what
    def __repr__(self):
        return 'optional(%s)' % repr(self.what)

class XcodeObject(object):
    COMPACT = False

    def _allobjs(self, dest):
        if self in dest:
            return
        dest.add(self)
        for attr in self.ATTRS:
            if isinstance(attr, optional):
                attr = attr.what
            v = getattr(self, attr)
            if isinstance(v, list):
                for x in v:
                    if isinstance(x, XcodeObject):
                        x._allobjs(dest)
            elif isinstance(v, dict):
                for x in v.itervalues():
                    if isinstance(x, XcodeObject):
                        x._allobjs(dest)
            elif isinstance(v, XcodeObject):
                v._allobjs(dest)

    def allobjs(self):
        objs = set([])
        self._allobjs(objs)
        return objs

    def emit(self, w, ids):
        def objemit(x):
            if isinstance(x, dict):
                w.start_dict()
                for k, v in x.iteritems():
                    w.write_key(k)
                    objemit(v)
                w.end_dict()
            elif isinstance(x, list):
                w.start_list()
                for v in x:
                    objemit(v)
                w.end_list()
            elif isinstance(x, XcodeObject):
                w.write_object(ids[x])
            else:
                w.write_object(x)
        w.write_key(ids[self])
        w.start_dict(self.COMPACT)
        w.write_pair('isa', self.ISA)
        for attr in self.ATTRS:
            if isinstance(attr, optional):
                attr = attr.what
                isoptional = True
            else:
                isoptional = False
            val = getattr(self, attr)
            if val is None:
                if not isoptional:
                    raise Exception('Missing %r attribiute from %r class' %
                        (attr, self.__class__.__name__))
            else:
                w.write_key(attr)
                objemit(val)
        w.end_dict()

def relpath(p, x):
    # This function doesn't work for all inputs
    p = p.split('/') if p else []
    x = x.split('/') if x else []
    while p and x and p[0] == x[0]:
        p.pop(0)
        x.pop(0)
    return '/'.join(['..' for i in x] + p)

class PathObject(XcodeObject):
    def __init__(self, path, root=None, name=None):
        self.parent = None
        path = path or ''
        self.abspath = path
        self.name = name
        if root is None:
            if path.startswith('/'):
                root = '<absolute>'
            else:
                root = 'SOURCE_ROOT'
        self.root = root
        self.make_relative()
    def make_relative(self):
        """Make the path of this object relative, if possible."""
        op = self.abspath
        if self.parent:
            pp = self.parent.abspath
            pr = self.parent.root
        elif self.root == 'SOURCE_ROOT':
            pp = ''
            pr = 'SOURCE_ROOT'
        else:
            pp = ''
            pr = ''
        if self.root == pr:
            self.path = relpath(op, pp)
            self.sourceTree = '<group>'
        else:
            self.path = op
            self.sourceTree = self.root

    def __cmp__(self, other):
        if isinstance(other, PathObject):
            x = cmp(self.CSORT, other.CSORT)
            if x: return x
            x = cmp(self.root, other.root)
            if x: return x
            x = cmp(self.abspath, other.abspath)
            if x: return x
            n = getattr(self, 'name', None)
            m = getattr(other, 'name', None)
            if n is None:
                if m is None:
                    return 0
                return -1
            else:
                if m is None:
                    return 1
                return cmp(n, m)
        return NotImplemented

class Group(PathObject):
    CSORT = 0
    ISA = 'PBXGroup'
    ATTRS = ['children', optional('name'), optional('path'), 'sourceTree']
    def __init__(self, path, root=None, name=None):
        PathObject.__init__(self, path, root=root, name=name)
        self.children = []
    def add(self, obj):
        if not isinstance(obj, PathObject):
            raise TypeError('Can only add PathObject to Group')
        if obj.parent is not None:
            raise ValueError('Can only add object to one group')
        self.children.append(obj)
        obj.parent = self
        obj.make_relative()

class FileRef(PathObject):
    CSORT = 1
    COMPACT = True
    ISA = 'PBXFileReference'
    ATTRS = [optional('fileEncoding'), optional('lastKnownFileType'),
             optional('explicitFileType'), optional('includeInIndex'),
             optional('name'), 'path', 'sourceTree']
    def __init__(self, path, root=None, name=None, etype=None):
        PathObject.__init__(self, path, root=root, name=name)
        ext = posixpath.splitext(path)[1]
        if etype is None:
            try:
                ftype = buildtool.path.EXTS[ext]
            except KeyError:
                ftype = ext[1:] if ext else ''
            ftype = TYPES[ftype]
            self.lastKnownFileType = ftype
            self.explicitFileType = None
        else:
            self.lastKnownFileType = None
            self.explicitFileType = etype
            ftype = etype
        self.includeInIndex = None
        if ftype.startswith('sourcecode.') or ftype.startswith('.text'):
            self.fileEncoding = 4
        else:
            self.fileEncoding = None
    @property
    def ftype(self):
        return self.explicitFileType or self.lastKnownFileType

class BuildFile(XcodeObject):
    COMPACT = True
    ISA = 'PBXBuildFile'
    ATTRS = ['fileRef']
    def __init__(self, f):
        self.fileRef = f

class BuildPhase(XcodeObject):
    ATTRS = ['buildActionMask', 'files', 'runOnlyForDeploymentPostprocessing']
    buildActionMask = 0x7fffffff
    runOnlyForDeploymentPostprocessing = 0
    def __init__(self):
        self.files = []
    def add(self, x):
        self.files.append(x)

class FrameworksBuildPhase(BuildPhase):
    ISA = 'PBXFrameworksBuildPhase'

class ResourcesBuildPhase(BuildPhase):
    ISA = 'PBXResourcesBuildPhase'

class SourcesBuildPhase(BuildPhase):
    ISA = 'PBXSourcesBuildPhase'

class ShellScriptBuildPhase(BuildPhase):
    ISA = 'PBXShellScriptBuildPhase'
    ATTRS = BuildPhase.ATTRS + ['inputPaths', 'outputPaths', 'shellPath', 'shellScript',
                                'showEnvVarsInLog']
    shellPath = '/bin/sh'
    showEnvVarsInLog = 0
    def __init__(self, script):
        BuildPhase.__init__(self)
        self.shellScript = script
        self.inputPaths = []
        self.outputPaths = []

class NativeTarget(XcodeObject):
    ISA = 'PBXNativeTarget'
    ATTRS = ['buildConfigurationList', 'buildPhases',
             'buildRules', 'dependencies', 'name',
             'productName', 'productReference', 'productType']
    def __init__(self, ttype, name, productName=None):
        productName = productName or name
        self.buildConfigurationList = None # Object
        self.buildPhases = [] # Object list
        self.buildRules = []
        self.dependencies = []
        self.name = name
        self.productName = productName
        product = FileRef(productName + '.app', root='BUILT_PRODUCTS_DIR',
                          etype='wrapper.application')
        self.productReference = product
        self.productType = ttype

class BuildConfigList(XcodeObject):
    ISA = 'XCConfigurationList'
    ATTRS = ['buildConfigurations', 'defaultConfigurationIsVisible',
             'defaultConfigurationName']
    def __init__(self, items, default=None):
        self.buildConfigurations = list(items)
        self.defaultConfigurationIsVisible = 0
        self.defaultConfigurationName = default or item[0].name

class BuildConfiguration(XcodeObject):
    ISA = 'XCBuildConfiguration'
    ATTRS = ['buildSettings', 'name']
    def __init__(self, name, *init):
        s = {}
        for obj in init:
            if isinstance(obj, BuildConfiguration):
                obj = obj.buildSettings
            s.update(obj)
        self.buildSettings = s
        self.name = name
    def __getitem__(self, i):
        return self.buildSettings[i]
    def __setitem__(self, i, v):
        self.buildSettings[i] = v
    def __delitem__(self, i):
        del self.buildSettings[i]
    def update(self, d):
        if isinstance(d, BuildConfiguration):
            d = d.buildSettings
        self.buildSettings.update(d)

class Project(XcodeObject):
    ISA = 'PBXProject'
    ATTRS = ['buildConfigurationList', 'compatibilityVersion',
             'hasScannedForEncodings', 'mainGroup', 'productRefGroup',
             'projectDirPath', 'projectRoot', 'targets']
    def __init__(self):
        self.buildConfigurationList = BuildConfigList
        self.compatibilityVersion = 'Xcode 2.4'
        self.hasScannedForEncodings = 0
        self.mainGroup = Group(None)
        self.productRefGroup = Group(None, name='Products')
        self.mainGroup.add(self.productRefGroup)
        self.projectDirPath = ''
        self.projectRoot = ''
        self.targets = []

class Xcode(object):
    def __init__(self):
        self._paths = {} # object for each path
        self._buildfile = {}
        self.project = Project()
        self._frameworks = Group(None, name='Frameworks')
        self.project.mainGroup.add(self._frameworks)

    def sort_groups(self):
        for x in self._paths.itervalues():
            if isinstance(x, Group):
                x.children.sort()

    def source_group(self, path):
        """Get the Group for the given directory."""
        if path.startswith('/'):
            raise Exception('No groups for absolute paths (path=%r)' % path)
        try:
            obj = self._paths[path]
        except KeyError:
            if path:
                obj = Group(path)
                par = self.source_group(posixpath.dirname(path))
                par.add(obj)
            else:
                obj = self.project.mainGroup
            self._paths[path] = obj
        else:
            if not isinstance(obj, Group):
                raise Exception('Group and FileRef have same path, %r' % path)
        return obj

    def build_file(self, f):
        try:
            return self._buildfile[f]
        except KeyError:
            obj = BuildFile(f)
            self._buildfile[f] = obj
            return obj

    def source_file(self, path, name=None):
        """Get the FileRef for the given file."""
        try:
            obj = self._paths[path]
        except KeyError:
            obj = FileRef(path, name=name)
            if obj.root == 'SOURCE_ROOT':
                par = self.source_group(posixpath.dirname(path))
            elif obj.ftype == 'wrapper.framework':
                par = self._frameworks
            else:
                assert 0
            par.add(obj)
            self._paths[path] = obj
        else:
            if not isinstance(obj, FileRef):
                raise Exception('Group and FileRef have same path, %r' % path)
        return obj

    def add_target(self, t):
        self.project.targets.append(t)
        self.project.productRefGroup.add(t.productReference)

    def write(self, f):
        self.sort_groups()
        objs = self.project.allobjs()
        groups = {}
        for obj in objs:
            try:
                g = groups[obj.ISA]
            except KeyError:
                groups[obj.ISA] = [obj]
            else:
                g.append(obj)
        idxs = dict(zip(ISAS, ['%03d' % x for x in range(len(ISAS))]))
        groups = list(groups.iteritems())
        groups.sort(key=lambda x: idxs.get(x[0], '999' + x[0]))
        nonce = random.getrandbits(32)
        ids = {}
        ctr = 0
        for groupname, groupobjs in groups:
            groupobjs.sort()
            for obj in groupobjs:
                ids[obj] = 'CAFEBABE%08X%08X' % (nonce, ctr)
                ctr += 1
        w = buildtool.plist.Writer(f)
        w.start_dict()
        w.write_pair('archiveVersion', 1)
        w.write_pair('classes', {})
        w.write_pair('objectVersion', 42)
        w.write_key('objects')
        w.start_dict()
        for groupname, groupobjs in groups:
            if groupobjs:
                for obj in groupobjs:
                    obj.emit(w, ids)
        w.end_dict()
        w.write_pair('rootObject', ids[self.project])
        w.end_dict()

def projectConfig(obj):
    base = {
        'GCC_PRECOMPILE_PREFIX_HEADER': True,
        'GCC_PREFIX_HEADER': 'src/platform/macosx/GPrefix.h',
        'GCC_VERSION': '4.2',
        'WARNING_CFLAGS': ['-Wall', '-Wextra'],
        'HEADER_SEARCH_PATHS': ['$(HEADER_SEARCH_PATHS)'] + obj.incldirs,
    }
    debug = {
        'COPY_PHASE_STRIP': False,
    }
    release = {
        'COPY_PHASE_STRIP': True,
    }
    cd = BuildConfiguration('Debug', base, debug)
    cr = BuildConfiguration('Release', base, release)
    return BuildConfigList([cd, cr], 'Release')

def targetConfig(obj):
    base = {
        'ALWAYS_SEARCH_USER_PATHS': False,
        'FRAMEWORK_SEARCH_PATHS': [
            '$(inherited)',
            '$(FRAMEWORK_SEARCH_PATHS_QUOTED_FOR_TARGET_1)',
        ],
        'FRAMEWORK_SEARCH_PATHS_QUOTED_FOR_TARGET_1':
            '"$(SYSTEM_LIBRARY_DIR)/Frameworks/ApplicationServices.framework/Frameworks"',
        'GCC_DYNAMIC_NO_PIC': False,
        'GCC_ENABLE_FIX_AND_CONTINUE': True,
        'GCC_MODEL_TUNING': 'G5',
        'GCC_OPTIMIZATION_LEVEL': 0,
        'INFOPLIST_FILE': 'mac/Game-Info.plist',
        'INSTALL_PATH': '$(HOME)/Applications',
        'OTHER_LDFLAGS': [
                '-framework', 'Foundation',
                '-framework', 'AppKit',
                '-framework', 'OpenGL',
        ],
        'PREBINDING': False,
        'PRODUCT_NAME': obj.env.EXE_MAC,
    }
    debug = {
        'COPY_PHASE_STRIP': False,
        'GCC_ENABLE_FIX_AND_CONTINUE': False,
        'GCC_OPTIMIZATION_LEVEL': 0,
    }
    release = {
        'ARCHS': 'ppc i386',
        'COPY_PHASE_STRIP': True,
        'DEBUG_INFORMATION_FORMAT': 'dwarf-with-dsym',
        'ZERO_LINK': False,
        'GCC_ENABLE_FIX_AND_CONTINUE': True,
    }
    cd = BuildConfiguration('Debug', base, debug)
    cr = BuildConfiguration('Release', base, release)
    return BuildConfigList([cd, cr], 'Release')

def run(obj):
    x = Xcode()
    x.project.buildConfigurationList = projectConfig(obj)
    t = NativeTarget('com.apple.product-type.application',
                     obj.env.EXE_MAC)
    t.buildConfigurationList = targetConfig(obj)
    script = ShellScriptBuildPhase('exec "$PROJECT_DIR"/scripts/version.sh "$PROJECT_DIR" "$PROJECT_DIR"')
    rsrcs = ResourcesBuildPhase()
    srcs = SourcesBuildPhase()
    fwks = FrameworksBuildPhase()
    t.buildPhases = [script, rsrcs, srcs, fwks]
    def add_source(fref):
        ft = fref.lastKnownFileType
        if ft in SOURCE_BUILD:
            phase = srcs
        elif ft in RESOURCE_BUILD:
            phase = rsrcs
        elif ft in FRAMEWORK_BUILD:
            phase = fwks
        else:
            phase = None
        if phase is not None:
            bref = x.build_file(fref)
            phase.add(bref)
    for src in obj.all_sources():
        x.source_file(src)
    for src in obj.get_atoms('MACOSX', None):
        add_source(x.source_file(src))
    for fw in ['Foundation', 'AppKit', 'CoreServices', 'CoreVideo', 'Carbon']:
        add_source(x.source_file('/System/Library/Frameworks/%s.framework' % fw, name=fw))
    add_source(x.source_file('mac/Game-Info.plist'))
    add_source(x.source_file('mac/MainMenu.xib'))
    x.add_target(t)
    f = cStringIO.StringIO()
    x.write(f)
    obj.write_file(obj.env.PKG_FILENAME + '.xcodeproj/project.pbxproj',
                   f.getvalue())
