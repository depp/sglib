__all__ = [
    'OS', 'Project', 'BaseModule',
    'Module', 'ExternalLibrary', 'Executable',
    'BundledLibrary', 'PkgConfig', 'Framework', 'SdlConfig',
    'LibrarySearch', 'TestSource',
    'Feature', 'Implementation',
]

OS = frozenset(['linux', 'windows', 'osx'])

class Project(object):
    """A project is a set of modules and project-wide settings."""

    __slots__ = [
        'name', 'ident', 'filename', 'url', 'email', 'copyright',
        'cvar', 'modules', 'module_names',
        'module_path', 'lib_path',
        'feature',
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
        self.feature = []

    def add_module(self, module):
        if module.name is not None:
            if module.modid in self.module_names:
                raise ValueError('duplicate module name: %s' % module.modid)
            self.module_names[module.modid] = module
        self.feature.extend(module.feature)
        self.modules.append(module)

    def targets(self):
        """Iterate over all target modules."""
        for m in self.modules:
            if m.is_target:
                yield m

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

    __slots__ = ['modid', 'name', 'header_path',
                 'define', 'require', 'cvar', 'sources', 'feature']

    def __init__(self, modid):
        self.modid = modid
        self.name = None
        self.header_path = []
        self.define = []
        self.require = []
        self.cvar = []
        self.sources = []
        self.feature = []

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

class Module(BaseModule):
    """Simple module with a fixed location and sources."""

    __slots__ = BaseModule.__slots__

class ExternalLibrary(object):
    """Module which is an external library.

    A module has a number possible library sources.  Each library
    source specifies a method of locating the library, finding its
    header search paths, and linking the library into a program.
    """

    __slots__ = ['modid', 'name', 'sources']

    def __init__(self, modid):
        self.modid = modid
        self.name = None
        self.sources = []

    def __repr__(self):
        return '<ExternalLibrary %s>' % self.modid

    @property
    def is_target(self):
        return False

    @property
    def header_path(self):
        return []

    @property
    def define(self):
        return []

    @property
    def require(self):
        return []

    @property
    def cvar(self):
        return []

    @property
    def feature(self):
        return []

    def add_libsource(self, source):
        self.sources.append(source)

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

class PkgConfig(object):
    """Library source which uses pkg-config to find a library."""

    __slots__ = ['spec']

    def __init__(self, spec):
        self.spec = spec

class Framework(object):
    """Library source which is a Darwin framework."""

    __slots__ = ['name']

    def __init__(self, name):
        self.name = name

class SdlConfig(object):
    """Library source which uses sdl-config to find LibSDL."""

    __slots__ = ['version']

    def __init__(self, version):
        self.version = version

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
