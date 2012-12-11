__all__ = ['OS', 'Project', 'BaseModule',
           'Module', 'BundledLibrary', 'Executable']

OS = frozenset(['linux', 'windows', 'osx'])

class Project(object):
    """A project is a set of modules and project-wide settings."""
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

    def add_module(self, module):
        if module.name is not None:
            if module.name in self.module_names:
                raise ValueError('duplicate module name: %s' % module.name)
            self.module_names[module.name] = module
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
    """

    __slots__ = ['modid', 'name', 'header_path',
                 'define', 'require', 'cvar', 'sources']

    def __init__(self, modid):
        self.modid = modid
        self.name = None
        self.header_path = []
        self.define = []
        self.require = []
        self.cvar = []
        self.sources = []

    @property
    def is_target(self):
        """Whether this module is a buildable target."""
        return False

    def add_source(self, src):
        self.sources.append(src)

class Module(BaseModule):
    """Simple module with a fixed location and sources."""

    __slots__ = BaseModule.__slots__

class BundledLibrary(BaseModule):
    """Module which is an external library.

    The 'lib' directory will be searched for a directory with a name
    that matches 'libname', and that directory will be used as the
    module root.
    """

    __slots__ = BaseModule.__slots__ + ['libname']

    def __init__(self, modid, libname):
        super(BundledLibrary, self).__init__(modid)
        self.libname = libname

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
