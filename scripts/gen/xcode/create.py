import gen.xcode.obj as obj
import gen.path
from gen.env import Environment
import gen.atom as atom
import posixpath

# Map from file types to phases
def mkphases():
    phases = {
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
    }
    d = {}
    for k, v in phases.iteritems():
        for t in v:
            assert t not in d
            d[t] = k
    return d
PHASES = mkphases()
del mkphases

TYPES = {
    'c': 'sourcecode.c.c',
    'cxx': 'sourcecode.cpp.cpp',
    'h': 'sourcecode.c.h',
    'hxx': 'sourcecode.cpp.h',
    'm': 'sourcecode.c.objc',
    'framework': 'wrapper.framework',
    'xib': 'file.xib',
    'plist': 'text.plist.xml',
    'icns': 'image.icns',
}

class Group(object):
    """Object for creating an Xcode group of source files."""
    __slots__ = ['_group', '_gpath', '_paths', '_project']

    def __init__(self, project, group):
        gpath = group.relpath.posix
        if gpath == '.':
            gpath = ''
        self._group = group
        self._gpath = gpath
        self._paths = {}
        self._project = project

    def get_dir(self, path):
        """Get the Xcode group for the given directory.

        Creates the group and any parent groups if necessary.
        The directory must be relative to the group root.
        """
        try:
            x = self._paths[path]
        except KeyError:
            pass
        else:
            if not isinstance(x, obj.Group):
                raise Exception('group and FileRef have same path: %r' % (path,))
            return x
        if path:
            abspath = posixpath.join(self._gpath, path)
        else:
            abspath = self._gpath
        if path:
            x = obj.Group(abspath)
            if path.startswith('/'):
                raise Exception('no groups for absolute paths: %r' % (path,))
            par, fname = posixpath.split(path)
            if fname.startswith('.'):
                raise Exception('path component starts with period: %r' % (path,))
            y = self.get_dir(par)
            y.add(x)
        else:
            x = obj.Group(abspath, name=self._group.name)
            self._project.mainGroup.add(x)
        self._paths[path] = x
        return x

    def get_source(self, source):
        """Get the FileRef for a given source file.

        The source file's group should match this group.
        """
        path = source.grouppath.posix
        if self._group is not source.group:
            raise ValueError('Group.add: group mismatch')
        try:
            x = self._paths[path]
        except KeyError:
            pass
        else:
            if not isinstance(x, obj.FileRef):
                raise Exception('group and FileRef have same path: %r' % (path,))
            return x
        ft = source.sourcetype
        try:
            ltype = TYPES[ft]
        except KeyError:
            raise Exception('unknown file type %s for path %s' %
                            (ft, source.relpath.posix))
        x = obj.FileRef(source.relpath.posix, ltype=ltype)
        group = self.get_dir(posixpath.dirname(path))
        group.add(x)
        self._paths[path] = x
        return x

class Target(object):
    """Object for creating an Xcode target."""
    __slots__ = ['_phases', 'target']
    def __init__(self, target):
        self._phases = {}
        self.target = target

    def add_phase(self, name, phase):
        if name:
            if name in self._phases:
                raise Exception('duplicate phase %s in target' % (name,))
            self._phases[name] = phase
        self.target.buildPhases.append(phase)

    def add_source(self, ref):
        ftype = ref.ftype
        try:
            phase = self._phases[PHASES[ftype]]
        except KeyError:
            # Ignore files with no phase and files where the phase
            # doesn't exist in this target
            return
        x = obj.BuildFile(ref)
        phase.add(x)

class Project(object):
    """Object for creating an Xcode project."""
    __slots__ = ['_groups', '_sources', 'project']

    def __init__(self):
        p = obj.Project()
        p.mainGroup = obj.Group(None)
        p.productRefGroup = obj.Group(None, name='Products')
        p.mainGroup.add(p.productRefGroup)
        self._groups = {}
        self._sources = {}
        self.project = p

    def get_source(self, source):
        """Get the Xcode file reference for a given source file."""
        gname = source._group.name
        try:
            g = self._groups[gname]
        except KeyError:
            g = Group(self.project, source._group)
            self._groups[gname] = g
        return g.get_source(source)

    def application_target(self, name):
        pname = name
        pfile = obj.FileRef(pname + '.app', root='BUILT_PRODUCTS_DIR',
                            etype='wrapper.application')
        t = obj.NativeTarget('com.apple.product-type.application', name)
        t.productName = pname
        t.productReference = pfile
        x = Target(t)
        x.add_phase('resource', obj.ResourcesBuildPhase())
        x.add_phase('source', obj.SourcesBuildPhase())
        x.add_phase('framework', obj.FrameworksBuildPhase())
        self.project.targets.append(t)
        return x

    def write(self, f):
        for x in self.project.allobjs():
            if isinstance(x, obj.Group):
                x.children.sort()
        self.project.write(f)

def projectConfig(env):
    base = {
        'GCC_VERSION': '4.2',
        'WARNING_CFLAGS': ['-Wall', '-Wextra'],
    }
    debug = {
        'COPY_PHASE_STRIP': False,
    }
    release = {
        'COPY_PHASE_STRIP': True,
    }
    cd = obj.BuildConfiguration('Debug', base, debug)
    cr = obj.BuildConfiguration('Release', base, release)
    return obj.BuildConfigList([cd, cr], 'Release')

def targetConfig(env):
    base = {
        'ALWAYS_SEARCH_USER_PATHS': False,
        'GCC_DYNAMIC_NO_PIC': False,
        'GCC_ENABLE_FIX_AND_CONTINUE': True,
        'GCC_MODEL_TUNING': 'G5',
        'GCC_OPTIMIZATION_LEVEL': 0,
        'INFOPLIST_FILE': 'resources/mac/Info.plist',
        'INSTALL_PATH': '$(HOME)/Applications',
        'PREBINDING': False,
        'PRODUCT_NAME': env.EXE_MAC,
        'HEADER_SEARCH_PATHS':
            ['$(HEADER_SEARCH_PATHS)'] + [p for p in env.CPPPATH],
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
        'GCC_ENABLE_FIX_AND_CONTINUE': True,
    }
    cd = obj.BuildConfiguration('Debug', base, debug)
    cr = obj.BuildConfiguration('Release', base, release)
    return obj.BuildConfigList([cd, cr], 'Release')

def write_project(proj, userenv, f):
    """Write a project as an Xcode project to the given file."""

    p = Project()
    for source in proj.sourcelist.sources():
        p.get_source(source)
    p.project.buildConfigurationList = projectConfig(Environment(proj.env, userenv))

    def lookup(atom):
        return None
    atomenv = atom.AtomEnv([lookup], proj.env, userenv, 'MACOSX')
    srcenv = atom.SourceEnv(proj, atomenv)
    env = srcenv.unionenv()
    t = p.application_target(env.EXE_MAC)
    for source, source_env in srcenv:
        t.add_source(p.get_source(source))
    t.target.buildConfigurationList = targetConfig(env)

    p.write(f)
