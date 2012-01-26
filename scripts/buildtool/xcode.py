import buildtool.path
import buildtool.plist
import random
import os
import sys

def gen_id(self):
    c = self._ctr
    self._ctr = c + 1
    return '%16X%08X' % (self._nonce, c)

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
    def __init__(self, path):
        self.parent = None
        self.abspath = path or ''
        self.make_relative()
    def make_relative(self):
        """Make the path of this object relative, if possible."""
        op = self.abspath
        if self.parent:
            pp = self.parent.abspath
        else:
            pp = ''
        if op.startswith('/') == pp.startswith('/'):
            self.path = relpath(op, pp)
            self.sourceTree = '<group>'
        elif self.path.startswith('/'):
            self.sourceTree = '<absolute>'
        else:
            self.sourceTree = 'SOURCE_ROOT'

    def __cmp__(self, other):
        if isinstance(other, PathObject):
            return cmp(self.abspath, other.abspath)
        return NotImplemented

class Group(PathObject):
    ISA = 'PBXGroup'
    ATTRS = ['children', optional('name'), optional('path'), 'sourceTree']
    def __init__(self, path, name=None):
        PathObject.__init__(self, path)
        self.children = []
        self.name = name
    def add(self, obj):
        if not isinstance(obj, PathObject):
            raise TypeError('Can only add PathObject to Group')
        if obj.parent is not None:
            raise ValueError('Can only add object to one group')
        self.children.append(obj)
        obj.parent = self
        obj.make_relative()

class FileRef(PathObject):
    COMPACT = True
    ISA = 'PBXFileReference'
    ATTRS = [optional('fileEncoding'), 'lastKnownFileType', 'path', 'sourceTree']
    def __init__(self, path):
        PathObject.__init__(self, path)
        ext = os.path.splitext(path)[1]
        try:
            ftype = buildtool.path.EXTS[ext]
        except KeyError:
            ftype = ext[1:] if ext else ''
        ftype = TYPES[ftype]
        self.lastKnownFileType = ftype
        if ftype.startswith('sourcecode.') or ftype.startswith('.text'):
            self.fileEncoding = 4
        else:
            self.fileEncoding = None

class Xcode(object):
    def __init__(self):
        self._paths = {} # object for each path
        self._root = Group(None) # root group

    def source_group(self, path):
        """Get the Group for the given directory."""
        if path.startswith('/'):
            raise Exception('No groups for absolute paths (path=%r)' % path)
        try:
            obj = self._paths[path]
        except KeyError:
            if path:
                obj = Group(path)
                par = self.source_group(os.path.dirname(path))
                par.add(obj)
            else:
                obj = self._root
            self._paths[path] = obj
        else:
            if not isinstance(obj, Group):
                raise Exception('Group and FileRef have same path, %r' % path)
        return obj

    def source_file(self, path):
        """Get the FileRef for the given file."""
        try:
            obj = self._paths[path]
        except KeyError:
            obj = FileRef(path)
            par = self.source_group(os.path.dirname(path))
            par.add(obj)
            self._paths[path] = obj
        else:
            if not isinstance(obj, FileRef):
                raise Exception('Group and FileRef have same path, %r' % path)
        return obj

    def write(self, f):
        objs = self._root.allobjs()
        groups = {}
        for obj in objs:
            try:
                g = groups[obj.ISA]
            except KeyError:
                groups[obj.ISA] = [obj]
            else:
                g.append(obj)
        idxs = dict(zip(ISAS, ['%03d' for x in range(len(ISAS))]))
        groups = list(groups.iteritems())
        groups.sort(key=lambda x: idxs.get(x[0], '999' + x[0]))
        nonce = random.getrandbits(32)
        ids = {}
        ctr = 0
        for groupname, groupobjs in groups:
            groupobjs.sort()
            for obj in groupobjs:
                ids[obj] = '%08X%08X' % (nonce, ctr)
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
        w.end_dict()

def run(obj):
    x = Xcode()
    for src in obj._sources:
        fref = x.source_file(src.path)
    x.write(sys.stdout)
