# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
"""Objects which are part of an Xcode project."""
import sys
import collections
import inspect

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
ISA_ORDER = {isa: n for n, isa in enumerate(ISAS)}

def const(x):
    """Constant attributes."""
    def get(self):
        return x
    return property(get)

optional = collections.namedtuple('optional', 'what')

def objemit(w, x, ids):
    if isinstance(x, dict):
        w.start_dict()
        for k, v in x.items():
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
    __slots__ = []

    def __init__(self, **kw):
        for class_ in inspect.getmro(self.__class__):
            try:
                slots = class_.__slots__
            except AttributeError:
                continue
            for slot in slots:
                try:
                    val = kw[slot]
                except KeyError:
                    setattr(self, slot, None)
                else:
                    setattr(self, slot, val)
                    del kw[slot]
        if kw:
            raise TypeError('extra keyword arguments: {}'
                            .format(', '.join(repr(x) for x in kw)))

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
                    for x in v.values():
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
                    raise Exception(
                        'missing mandatory attribiute: {}.{}'
                        .format(self.isa, key))
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

class PathObject(XcodeObject):
    __slots__ = []

class Group(PathObject):
    """Group of files in Xcode project."""
    __slots__ = ['children', 'name', 'path', 'sourceTree']
    isa = const('PBXGroup')
    projectobj = const(True)
    userobj = const(False)

    def __init__(self, *, sourceTree, children=(), **kw):
        super(Group, self).__init__(
            sourceTree=sourceTree, children=list(children), **kw)

    def xkeys(self):
        return iter((
            'children', optional('name'),
            optional('path'), 'sourceTree'
        ))

class FileRef(PathObject):
    """A reference to a file."""
    __slots__ = ['fileEncoding', 'lastKnownFileType', 'explicitFileType',
                 'includeInIndex', 'name', 'path', 'sourceTree']
    isa = const('PBXFileReference')
    projectobj = const(True)
    userobj = const(False)

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
    __slots__ = ['fileRef', 'settings']
    isa = const('PBXBuildFile')
    projectobj = const(True)
    userobj = const(False)

    def xkeys(self):
        return iter(('fileRef', optional('settings')))

    def compact(self):
        return True

class BuildPhase(XcodeObject):
    """Abstract base for Xcode build phases.

    Each build phase will consist of a collection of BuildFile objects,
    as well as some extra metadata.
    """
    __slots__ = []
    projectobj = const(True)
    userobj = const(False)

    def __init__(self, *, files=(), buildActionMask=0x7fffffff,
                 runOnlyForDeploymentPostprocessing=0, **kw):
        super(BuildPhase, self).__init__(
            files=list(files),
            buildActionMask=buildActionMask,
            runOnlyForDeploymentPostprocessing=
                runOnlyForDeploymentPostprocessing,
            **kw)

    def xkeys(self):
        return iter((
            'buildActionMask', 'files',
            'runOnlyForDeploymentPostprocessing',
        ))

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

    def __init__(self, *, inputPaths=(), outputPaths=(),
                 shellPath='/bin/sh', showEnvVarsInLog=0, **kw):
        super(ShellScriptBuildPhase, self).__init__(
            inputPaths=list(inputPaths),
            outputPaths=list(outputPaths),
            shellPath=shellPath,
            showEnvVarsInLog=showEnvVarsInLog,
            **kw)

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

    def __init__(self, *, buildPhases=(), buildRules=(),
                 dependencies=(), **kw):
        super(NativeTarget, self).__init__(
            buildPhases=list(buildPhases),
            buildRules=list(buildRules),
            dependencies=list(dependencies),
            **kw)

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

    def __init__(self, *, buildConfigurations,
                 defaultConfigurationName,
                 defaultConfigurationIsVisible=0,
                 **kw):
        super(BuildConfigList, self).__init__(
            buildConfigurations=list(buildConfigurations),
            defaultConfigurationName=defaultConfigurationName,
            defaultConfigurationIsVisible=defaultConfigurationIsVisible,
            **kw)

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

    def __init__(self, *, buildSettings={}, **kw):
        super(BuildConfiguration, self).__init__(
            buildSettings=dict(buildSettings), **kw)

    def xkeys(self):
        return iter(('buildSettings', 'name'))

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

    def __init__(self, *, targets=(), **kw):
        super(Project, self).__init__(
            targets=list(targets), **kw)

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

    def write(self, pf, uf, *, objectVersion):
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
        groups = list(groups.items())
        def getidx(x):
            isa, obj = x
            try:
                return isasort[isa]
            except KeyError:
                print('warning: unknown isa %s' % (isa,), file=sys.stderr)
                return '999:' + isa
        groups.sort(key=getidx)

        # Create object IDs for every object
        import random
        nonce = random.getrandbits(32)
        ids = {}
        ctr = 0
        for groupname, groupobjs in groups:
            # groupobjs.sort()
            for obj in groupobjs:
                ids[obj] = 'CAFEBABE%08X%08X' % (nonce, ctr)
                ctr += 1

        # Write all objects
        import build.plist.ascii as plist

        w = plist.Writer(pf)
        w.start_dict()
        w.write_pair('archiveVersion', 1)
        w.write_pair('classes', {})
        w.write_pair('objectVersion', objectVersion)
        w.write_key('objects')
        w.start_dict()
        for groupname, groupobjs in groups:
            if groupobjs:
                for obj in groupobjs:
                    obj.xemit(w, ids)
        w.end_dict()
        w.write_pair('rootObject', ids[self])
        w.end_dict()

        w = plist.Writer(uf)
        w.start_dict()
        for groupname, groupobjs in groups:
            if groupobjs:
                for obj in groupobjs:
                    obj.uemit(w, ids)
        w.end_dict()
