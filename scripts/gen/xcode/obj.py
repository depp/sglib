"""Objects which are part of an Xcode project."""
import sys

# Sort order for class ISAs
ISAS = [
    'PBXBuildFile',
    'PBXFileReference',
    'PBXFrameworksBuildPhase',
    'PBXGroup',
    'PBXExecutable',
    'PBXNativeTarget',
    'PBXProject',
    'PBXResourcesBuildPhase',
    'PBXShellScriptBuildPhase',
    'PBXSourcesBuildPhase',
    'XCBuildConfiguration',
    'XCConfigurationList',
]

def const(x):
    """Constant attributes."""
    def get(self):
        return x
    return property(get)

class optional(object):
    """Optional keys in Xcode objects."""
    def __init__(self, what):
        self.what = what
    def __repr__(self):
        return 'optional(%s)' % repr(self.what)

def objemit(w, x, ids):
    if isinstance(x, dict):
        w.start_dict()
        for k, v in x.iteritems():
            w.write_key(k)
            objemit(w, v, ids)
        w.end_dict()
    elif isinstance(x, list):
        w.start_list()
        for v in x:
            objemit(w, v, ids)
        w.end_list()
    elif isinstance(x, XcodeObject):
        w.write_object(ids[x])
    else:
        w.write_object(x)

class XcodeObject(object):
    """Root Xcode object, abstract.

    Concrete subclasses should provide an 'xkeys' method which yields
    a list of attributes, or override the 'xitems' method.  The 'xkeys'
    property returns a list of attributes to be written to the file.  Each
    attribute is either a string or an instance of 'optional'.  The
    default 'xkeys' implementation uses 'attrs'.

    The 'isa' property should provide the isa for the object class.

    The 'userobj' property indicates that the object is emitted in the
    user file (default.pbxuser).  The 'projectobj' property indicates
    that the object is emitted in the project file (project.pbxproj).
    """

    def compact(self):
        """Returns whether this object should be formatted compactly.

        This does not affect the correctness of the output, only the
        readability.  Compact formatting puts the entire object on a
        single line in the project file.
        """
        return False

    def _allobjs(self, dest):
        if self in dest:
            return
        dest.add(self)
        for items in (self.xitems, self.uitems):
            for k, v in items():
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
        """Get a set of all objects reachable from this object."""
        objs = set([])
        self._allobjs(objs)
        return objs

    def xkeys(self):
        """Iterate over the project keys in the object.

        These are the keys for the project.pbxproj file.
        """
        return iter(())

    def ukeys(self):
        """Iterate over the user keys in the object.

        These are the keys for the default.pbxuser file.
        """
        return iter(())

    def _gitems(self, keys):
        for key in keys:
            if isinstance(key, optional):
                key = key.what
                isoptional = True
            else:
                isoptional = False
            val = getattr(self, key)
            if val is None:
                if not isoptional:
                    raise Exception('Missing %r attribiute from %r class' %
                        (attr, self.__class__.__name__))
            else:
                yield key, val

    def xitems(self):
        """Iterate over all (key, value) pairs in the object.

        This is used for serializing the object to the project file.
        Objects are serialized as dictionaries.  The default
        implementation uses the results of 'xkeys' and 'getattr'.
        This will raise an exception if any key in 'xkeys' which is
        not marked as optional has a None value.
        """
        return self._gitems(self.xkeys())

    def uitems(self):
        """As xitems, but for user keys and values."""
        return self._gitems(self.ukeys())

    def _gemit(self, w, ids, *itemlists):
        w.write_key(ids[self])
        w.start_dict(self.compact())
        for itemlist in itemlists:
            for key, val in itemlist:
                w.write_key(key)
                objemit(w, val, ids)
        w.end_dict()

    def xemit(self, w, ids):
        """Write this object to a project.

        The w parameter is the plist writer object.  The ids parameter is a
        map from xcode objects to object IDs in the resulting plist, and must
        be constructed in advance.
        """
        if self.projectobj:
            self._gemit(w, ids, [('isa', self.isa)], self.xitems())

    def uemit(self, w, ids):
        """As xemit, but for the user file."""
        if self.userobj:
            if self.projectobj:
                self._gemit(w, ids, self.uitems())
            else:
                self._gemit(w, ids, [('isa', self.isa)], self.uitems())

def relpath(p, x):
    # This function doesn't work for all inputs
    p = p.split('/') if p else []
    x = x.split('/') if x else []
    while p and x and p[0] == x[0]:
        p.pop(0)
        x.pop(0)
    return '/'.join(['..' for i in x] + p)

class PathObject(XcodeObject):
    """Abstract base for Xcode objects corresponding to paths.

    This is the superclass for both groups and source files.
    Path objects can be sorted relative to each other.
    Subclasses should provide a csort attribute, which is a key
    which takes precedence over file paths when sorting.
    """

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
        if not isinstance(other, PathObject):
            return NotImplemented
        x = cmp(self.csort, other.csort)
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

class Group(PathObject):
    """Group of files in Xcode project."""
    __slots__ = ['children', 'name', 'path', 'sourceTree']
    isa = const('PBXGroup')
    csort = const(0)
    projectobj = const(True)
    userobj = const(False)

    def __init__(self, path, root=None, name=None):
        PathObject.__init__(self, path, root=root, name=name)
        self.children = []

    def xkeys(self):
        return iter((
            'children', optional('name'),
            optional('path'), 'sourceTree'
        ))

    def add(self, obj):
        """Add a path object to this group.

        If possible, this will modify the child object so that its path
        is stored relative to the path of this object.
        """
        if not isinstance(obj, PathObject):
            raise TypeError('Can only add PathObject to Group')
        if obj.parent is not None:
            raise ValueError('Can only add object to one group')
        self.children.append(obj)
        obj.parent = self
        obj.make_relative()

class FileRef(PathObject):
    """A reference to a file."""
    __slots__ = ['fileEncoding', 'lastKnownFileType', 'explicitFileType',
                 'includeInIndex', 'name', 'path', 'sourceTree']
    isa = const('PBXFileReference')
    csort = const(0)
    projectobj = const(True)
    userobj = const(False)

    def __init__(self, path, root=None, name=None, etype=None, ltype=None):
        # etype -> explicit file type
        # ltype -> last known file type
        PathObject.__init__(self, path, root=root, name=name)
        if ltype is not None:
            if etype is not None:
                raise ValueError('only one of etype, ltype should be specified')
            ftype = ltype
        elif etype is not None:
            ftype = etype
        else:
            raise ValueError('one of etype, ltype must be specified')
        if ftype.startswith('sourcecode.') or ftype.startswith('.text'):
            self.fileEncoding = 4
        else:
            self.fileEncoding = None
        self.lastKnownFileType = ltype
        self.explicitFileType = etype
        self.includeInIndex = None

    def compact(self):
        return True

    def xkeys(self):
        return iter((
            optional('fileEncoding'), optional('lastKnownFileType'),
            optional('explicitFileType'), optional('includeInIndex'),
            optional('name'), 'path', 'sourceTree',
        ))

    @property
    def ftype(self):
        return self.explicitFileType or self.lastKnownFileType

class BuildFile(XcodeObject):
    """Files used in a build process.
    
    Consists of a reference to a file reference.
    """
    __slots__ = ['fileRef']
    isa = const('PBXBuildFile')
    projectobj = const(True)
    userobj = const(False)

    def __init__(self, f):
        self.fileRef = f

    def xkeys(self):
        return iter(('fileRef',))

    def compact(self):
        return True

class BuildPhase(XcodeObject):
    """Abstract base for Xcode build phases.

    Each build phase will consist of a collection of BuildFile objects,
    as well as some extra metadata.
    """
    projectobj = const(True)
    userobj = const(False)

    def __init__(self):
        self.files = []
        self.buildActionMask = 0x7fffffff
        self.runOnlyForDeploymentPostprocessing = 0

    def xkeys(self):
        return iter((
            'buildActionMask', 'files',
            'runOnlyForDeploymentPostprocessing',
        ))

    def add(self, x):
        self.files.append(x)

class FrameworksBuildPhase(BuildPhase):
    """Build phase which links an executable.

    Build files are frameworks and libraries to link into the
    executable.
    """
    __slots__ = ['files', 'buildActionMask',
                 'runOnlyForDeploymentPostprocessing']
    isa = const('PBXFrameworksBuildPhase')

class ResourcesBuildPhase(BuildPhase):
    """Build phase which copies and compiles resources.

    Build files are resources to copy into the application.
    """
    __slots__ = ['files', 'buildActionMask',
                 'runOnlyForDeploymentPostprocessing']
    isa = const('PBXResourcesBuildPhase')

class SourcesBuildPhase(BuildPhase):
    """Build phase which compiles source files.

    Build files are source code files, not including headers.
    """
    __slots__ = ['files', 'buildActionMask',
                 'runOnlyForDeploymentPostprocessing']
    isa = const('PBXSourcesBuildPhase')

class ShellScriptBuildPhase(BuildPhase):
    """Build phase which executes a shell script.

    Takes a single argument, which is the shell script to run.
    """
    __slots__ = [
        'files', 'buildActionMask', 'runOnlyForDeploymentPostprocessing',
        'inputPaths', 'outputPaths', 'shellPath',
        'shellScript', 'showEnvVarsInLog',
    ]
    isa = const('PBXShellScriptBuildPhase')

    def __init__(self, script):
        BuildPhase.__init__(self)
        self.shellScript = script
        self.inputPaths = []
        self.outputPaths = []
        self.shellPath = '/bin/sh'
        self.showEnvVarsInLog = 0

    def xkeys(self):
        for k in BuildPhase.xkeys(self):
            yield k
        ks = (
            'inputPaths', 'outputPaths', 'shellPath',
            'shellScript', 'showEnvVarsInLog',
        )
        for k in ks:
            yield k

class NativeTarget(XcodeObject):
    """Target which builds an executable."""
    __slots__ = ['buildConfigurationList', 'buildPhases', 'buildRules',
                 'dependencies', 'name', 'productName', 'productReference',
                 'productType', 'executables']
    isa = const('PBXNativeTarget')
    projectobj = const(True)
    userobj = const(True)

    def __init__(self, ttype, name):
        self.buildConfigurationList = None # Object
        self.buildPhases = [] # Object list
        self.buildRules = []
        self.dependencies = []
        self.name = name
        self.productName = None
        self.productReference = None
        self.productType = ttype
        self.executables = None

    def xkeys(self):
        return iter((
            'buildConfigurationList', 'buildPhases',
            'buildRules', 'dependencies', 'name',
            'productName', 'productReference', 'productType'
        ))

    def ukeys(self):
        return iter((
            optional('executables'),
        ))

class BuildConfigList(XcodeObject):
    """List of build configurations (e.g., debug, release)."""
    __slots__ = ['buildConfigurations', 'defaultConfigurationIsVisible',
                 'defaultConfigurationName']
    isa = const('XCConfigurationList')
    projectobj = const(True)
    userobj = const(False)

    def __init__(self, items, default=None):
        self.buildConfigurations = list(items)
        self.defaultConfigurationIsVisible = 0
        self.defaultConfigurationName = default or item[0].name

    def xkeys(self):
        return iter((
            'buildConfigurations', 'defaultConfigurationIsVisible',
            'defaultConfigurationName'
        ))

class BuildConfiguration(XcodeObject):
    """A build configuration (e.g., debug, release).
    
    This is basically a dictionary of values.  It can be used as a
    dictionary.
    """
    __slots__ = ['buildSettings', 'name']
    isa = const('XCBuildConfiguration')
    projectobj = const(True)
    userobj = const(False)

    def __init__(self, name, *init):
        s = {}
        for obj in init:
            if isinstance(obj, BuildConfiguration):
                obj = obj.buildSettings
            s.update(obj)
        self.buildSettings = s
        self.name = name

    def xkeys(self):
        return iter(('buildSettings', 'name'))

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

    def keys(self):
        return self.buildSettings.keys()

    def items(self):
        return self.buildSettings.items()

    def values(self):
        return self.buildSettings.values()

class Executable(XcodeObject):
    """Information about an executable.

    This only appears in the user file.
    """
    EKEYS = [
        'activeArgIndices',
        'argumentStrings',
        #'autoAttachOnCrash',
        #'breakpointsEnabled',
        #'configStateDict',
        #'customDataFormattersEnabled',
        #'debuggerPlugin',
        #'disassemblyDisplayState',
        #'dylibVariantSuffix',
        #'enableDebugStr',
        #'environmentEntries',
        #'executableSystemSymbolLevel',
        #'executableUserSymbolLevel',
        #'libgmallocEnabled',
        'name',
        #'sourceDirectories',
    ]
    __slots__ = EKEYS
    isa = const('PBXExecutable')
    projectobj = const(False)
    userobj = const(True)

    def __init__(self, name):
        self.activeArgIndices = []
        self.argumentStrings = []
        self.name = name

    def ukeys(self):
        return iter(Executable.EKEYS)

class Project(XcodeObject):
    """An Xcode project object.
    
    This will be the root object of the project file.
    """
    __slots__ = ['buildConfigurationList', 'compatibilityVersion',
                 'hasScannedForEncodings', 'mainGroup', 'productRefGroup',
                 'projectDirPath', 'projectRoot', 'targets',
                 'executables']
    isa = const('PBXProject')
    projectobj = const(True)
    userobj = const(True)

    def __init__(self):
        self.buildConfigurationList = None
        self.compatibilityVersion = 'Xcode 2.4'
        self.hasScannedForEncodings = 0
        self.mainGroup = None
        self.productRefGroup = None
        self.projectDirPath = ''
        self.projectRoot = ''
        self.targets = []
        self.executables = None

    def xkeys(self):
        return iter((
            'buildConfigurationList', 'compatibilityVersion',
            'hasScannedForEncodings', 'mainGroup', 'productRefGroup',
            'projectDirPath', 'projectRoot', 'targets',
        ))

    def ukeys(self):
        return iter((
            optional('executables'),
        ))

    def write(self, pf, uf):
        """Write this project to the given files.

        The first file is the project file, the second file is the user
        defaults file.
        """
        objs = self.allobjs()

        # Group objects in the output by isa
        # this makes the output MUCH easier to read and debug
        # (it's also how Xcode does it)
        groups = {}
        for obj in objs:
            isa = obj.isa
            try:
                g = groups[isa]
            except KeyError:
                groups[isa] = [obj]
            else:
                g.append(obj)

        # Sort according to the ISA list
        isasort = dict(zip(ISAS, ['%03d' % x for x in range(len(ISAS))]))
        groups = list(groups.iteritems())
        def getidx(x):
            isa, obj = x
            try:
                return isasort[isa]
            except KeyError:
                print >>sys.stderr, 'warning: unknown isa %s' % (isa,)
                return '999:' + isa
        groups.sort(key=getidx)

        # Create object IDs for every object
        import random
        nonce = random.getrandbits(32)
        ids = {}
        ctr = 0
        for groupname, groupobjs in groups:
            groupobjs.sort()
            for obj in groupobjs:
                ids[obj] = 'CAFEBABE%08X%08X' % (nonce, ctr)
                ctr += 1

        # Write all objects
        import gen.plist

        w = gen.plist.Writer(pf)
        w.start_dict()
        w.write_pair('archiveVersion', 1)
        w.write_pair('classes', {})
        w.write_pair('objectVersion', 42)
        w.write_key('objects')
        w.start_dict()
        for groupname, groupobjs in groups:
            if groupobjs:
                for obj in groupobjs:
                    obj.xemit(w, ids)
        w.end_dict()
        w.write_pair('rootObject', ids[self])
        w.end_dict()

        w = gen.plist.Writer(uf)
        w.start_dict()
        for groupname, groupobjs in groups:
            if groupobjs:
                for obj in groupobjs:
                    obj.uemit(w, ids)
        w.end_dict()