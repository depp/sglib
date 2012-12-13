__all__ = [
    'OS', 'Project', 'BaseModule',
    'Module', 'Intrinsic', 'ExternalLibrary', 'Executable',
    'BundledLibrary', 'LibraryGroup', 'PkgConfig', 'Framework', 'SdlConfig',
    'LibrarySearch', 'TestSource',
    'Feature', 'Implementation', 'Variant', 'Defaults',
]
from gen.path import common_ancestor

# Intrinsics for each OS
OS = {
    'LINUX': ('LINUX', 'POSIX'),
    'OSX': ('OSX', 'POSIX'),
    'WINDOWS': ('WINDOWS',),
}
INTRINSICS = set(x for v in OS.values() for x in v)

class Project(object):
    """A project is a set of modules and project-wide settings."""

    __slots__ = [
        'name', 'ident', 'filename', 'url', 'email', 'copyright',
        'cvar', 'modules', 'module_names',
        'module_path', 'lib_path', 'defaults',
    ]

    def __init__(self):
        self.name = None
        self.ident = None
        self.filename = None
        self.url = None
        self.email = None
        self.copyright = None
        self.cvar = []
        self.modules = []
        self.module_names = {}
        self.module_path = []
        self.lib_path = None
        self.defaults = []

    def add_module(self, module):
        if module.modid is not None:
            if module.modid in self.module_names:
                raise ValueError('duplicate module name: %s' % module.modid)
            self.module_names[module.modid] = module
        self.modules.append(module)

    def targets(self):
        """Iterate over all target modules."""
        for m in self.modules:
            if m.is_target:
                yield m

    def closure(self, modules):
        """Get a list of all modules the given modules depend on.

        This returns (mods, unsat), where mods is a list of the
        modules (including the original modules), and unsat is a list
        of tags which are unsatisfied.
        """
        used = set()
        q = list(modules)
        unsat = []
        mods = list(q)
        while q:
            m = q.pop()
            deps = set(m.require)
            for f in m.feature:
                for i in f.impl:
                    deps.update(i.require)
            for v in m.variant:
                deps.update(v.require)
            for mid in deps.difference(used):
                try:
                    m = self.module_names[mid]
                except KeyError:
                    unsat.append(mid)
                else:
                    mods.append(m)
                    q.append(m)
            used.update(deps)
        return mods, unsat

    def features(self, modules=None):
        """Get a list of features for the given modules.

        If no iterable of modules is supplied, then all features are
        returned.
        """
        if modules is not None:
            mods, unsat = self.closure(modules)
        else:
            mods = self.modules
        a = []
        for m in mods:
            a.extend(m.feature)
        return a

    def variants(self, targets=None):
        """Get a list of variants for the given modules.

        If no iterable of modules is supplied, then all variants are
        returned.
        """
        if modules is not None:
            mods, unsat = self.closure(modules)
        else:
            mods = self.modules
        a = []
        for m in mods:
            a.extend(m.variant)
        return a

    def sources(self, target, getenv):
        """Iterate over the sources in a given target.

        Yields (source, env) pairs, where env is the environment for
        building the given source.  The environment is taken from the
        getenv function, which should map a module name and a set of
        tags to either the corresponding environment if the source
        should be built, or None if the source should not be built.
        """
        modules = set()
        mlist = [target]

        while mlist:
            m = mlist.pop()

class BaseModule(object):
    """A module.

    A project module is a group of sources.  Modules can depend on
    other modules, so when one module is built, dependent modules will
    also be built and linked in.  All paths are relative to the module
    root, and the module root is a native path.  Concrete module
    subclasses can be just source code, or they can be targets such as
    executables.

    name: identifies the module

    desc: human-readable description

    header_path: list of header include paths, both for this module
    and any source depending on it, including indirect dependencies

    define: list of preprocessor definitions, both for this module and
    any source depending on it, including indirect dependencies

    require: list of modules this module depends on

    cvar: list of cvars and values to be set by default for developers

    sources: list of source files

    feature: list of features this module has
    """

    __slots__ = [
        'modid', 'name',
        'header_path', 'define', 'require', 'cvar',
        'sources', 'feature', 'variant', 'defaults',
    ]

    def __init__(self, modid):
        self.modid = modid
        self.name = None
        self.header_path = []
        self.define = []
        self.require = []
        self.cvar = []
        self.sources = []
        self.feature = []
        self.variant = []
        self.defaults = []

    def __repr__(self):
        if self.modid is not None:
            return '<%s %s>' % (self.__class__.__name__, self.modid)
        else:
            return '<%s %#x>' % (self.__class__.__name__, id(self))

    @property
    def is_target(self):
        """Whether this module is a buildable target."""
        return False

    def add_source(self, src):
        self.sources.append(src)

    def module_root(self):
        """Get the directory containing all module sources.

        This will throw an exception for modules containing no sources
        (e.g., external libraries).
        """
        return common_ancestor(src.path for src in self.sources)

class Module(BaseModule):
    """Simple module with a fixed location and sources."""

    __slots__ = BaseModule.__slots__

class Intrinsic(BaseModule):
    """Module which is intrinsically present or not present.

    There is no XML representation for this kind of module.
    """

    __slots__ = BaseModule.__slots__

    def __repr__(self):
        return '<Intrinsic %s>' % self.modid

class ExternalLibrary(BaseModule):
    """Module which is an external library.

    A module has a number possible library sources.  Each library
    source specifies a method of locating the library, finding its
    header search paths, and linking the library into a program.

    When the project is configured or reconfigured, the library
    directory will be searched for bundled libraries.  Any library
    which is found will be used to fill in the base module properties:
    header_path, sources, et cetera, and the use_bundled attribute
    will be set to True.
    """

    __slots__ = (BaseModule.__slots__ +
                 ['libsources', 'bundled_versions', 'use_bundled'])

    def __init__(self, modid):
        super(ExternalLibrary, self).__init__(modid)
        self.libsources = []
        self.bundled_versions = []
        self.use_bundled = False

    def __repr__(self):
        return '<ExternalLibrary %s>' % self.modid

    def add_libsource(self, source):
        self.libsources.append(source)

class BundledLibrary(object):
    """Library source for libraries bundled with the project.

    The 'lib' directory will be searched for a directory matching the
    specified 'libname', and that library will be used as the root
    path.  A bundled library is otherwise similar to an ordinary
    module.
    """

    __slots__ = ['libname', 'header_path', 'define',
                 'require', 'cvar', 'sources']

    def __init__(self, libname):
        self.libname = libname
        self.header_path = []
        self.define = []
        self.require = []
        self.cvar = []
        self.sources = []

    def add_source(self, source):
        self.sources.append(source)

class LibraryGroup(object):
    """Library source which aggregates sources.

    All library sources in the group must be present.  Bundled
    libraries can not be included inside the group.
    """

    __slots__ = ['libsources']

    def __init__(self):
        self.libsources = []

    def add_libsource(self, source):
        self.libsources.append(source)

class PkgConfig(object):
    """Library source which uses pkg-config to find a library."""

    __slots__ = ['spec']

    def __init__(self, spec):
        self.spec = spec

    srcname = 'pkgconfig'

class Framework(object):
    """Library source which is a Darwin framework."""

    __slots__ = ['name']

    def __init__(self, name):
        self.name = name

    srcname = 'framework'

class SdlConfig(object):
    """Library source which uses sdl-config to find LibSDL."""

    __slots__ = ['version']

    def __init__(self, version):
        self.version = version

    srcname = 'sdlconfig'

class LibrarySearch(object):
    """Library source which searches for a library.

    This creates a source file, and then tries a series of different
    compilation flags until it finds a set that can compile and link
    the source file.
    """

    __slots__ = ['source', 'flags']

    def __init__(self, source, flags):
        self.source = source
        self.flags = flags

    srcname = 'librarysearch'

class TestSource(object):
    """Test source file.

    Consists of a prologue and a body.  The prologue is placed at the
    top of the source file.  Below the prologue the main() function is
    defined, which contains the body followed by a return statement.
    """

    __slots__ = ['prologue', 'body']

    def __init__(self, prologue, body):
        self.prologue = prologue
        self.body = body

class Executable(Module):
    """Module that produces an executable."""

    __slots__ = (BaseModule.__slots__ +
                 ['exe_name', 'exe_icon', 'apple_category'])

    @property
    def is_target(self):
        return True

    def __init__(self, modid):
        super(Executable, self).__init__(modid)
        self.exe_name = {}
        self.exe_icon = {}
        self.apple_category = None

class Feature(object):
    """A feature is a part of the code that can be enabled or disabled.

    A feature can have multiple imlementations.  Exactly one
    implementation can be chosen.  The implementation is chosen during
    the configuration process.
    """

    __slots__ = ['modid', 'desc', 'impl']

    def __init__(self, modid):
        self.modid = modid
        self.desc = None
        self.impl = []

    def __repr__(self):
        return '<Feature %s>' % self.modid

class Implementation(object):
    """A set of modules required for an implementation."""

    __slots__ = ['require', 'provide']

    def __init__(self, require, provide):
        self.require = require
        self.provide = provide

class Variant(object):
    """A variant is a different version of the code that can be built.

    Multiple variants can be built in parallel, but only one will ever
    be linked into the same executable.
    """

    __slots__ = ['varname', 'name', 'require']

    def __init__(self, varname):
        self.varname = varname
        self.name = None
        self.require = []

class Defaults(object):
    """A defaults object controls the default libraries to use."""

    __slots__ = ['os', 'variants', 'libs', 'features']

    def __init__(self, os, variants, libs, features):
        self.os = os
        self.variants = variants
        self.libs = libs
        self.features = features
