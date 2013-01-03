__all__ = ['Executable', 'Module', 'ExternalLibrary', 'BundledLibrary']
from gen.project.source import SourceModule

class Executable(object):
    """An executable target."""

    __slots__ = ['name', 'filename', 'exe_icon', 'apple_category',
                 'config', 'srcmodule']

    def __init__(self):
        self.name = None
        self.filename = {}
        self.exe_icon = []
        self.apple_category = None
        self.srcmodule = SourceModule()

    def add_source(self, src):
        self.srcmodule.add_source(src)

    def add_config(self, config):
        self.srcmodule.add_config(config)

    def module_deps(self):
        return self.srcmodule.module_deps()

class Module(object):
    """A simple module, consisting of project source code."""

    __slots__ = ['name', 'srcmodule']

    def __init__(self):
        self.name = None
        self.srcmodule = SourceModule()

    def add_source(self, src):
        self.srcmodule.add_source(src)

    def add_config(self, config):
        self.srcmodule.add_config(config)

    def scan_bundled_libs(self, paths):
        pass

    def module_deps(self):
        return self.srcmodule.module_deps()

class ExternalLibrary(object):
    """A module which can be incorporated into a target multiple ways.

    For example, it could be found with pkg-config or bundled with the
    project.  Each method of incorporating the module into the project
    is a "library source".
    """

    __slots__ = ['libsources', 'bundles', 'srcmodule', 'name']

    def __init__(self):
        self.libsources = []
        self.bundles = []
        self.srcmodule = None
        self.name = None

    def add_config(self, config):
        if isinstance(config, BundledLibrary):
            self.bundles.append(config)
        else:
            self.libsources.append(config)

    def scan_bundled_libs(self, paths):
        for bundle in self.bundles:
            srcmodule = bundle.scan_bundled_libs(paths)
            if srcmodule is not None:
                self.srcmodule = srcmodule
                break
        self.bundles = []

    def module_deps(self):
        mod_deps = set()
        if self.srcmodule is not None:
            mod_deps.update(self.srcmodule.module_deps())
        for source in self.libsources:
            mod_deps.update(source.module_deps())
        return mod_deps

class BundledLibrary(object):
    """Library source for libraries bundled with the project.

    The 'lib' directory will be searched for a directory matching the
    library name, and that library will be used as the root path.  A
    bundled library is a source module.
    """

    __slots__ = ['srcmodule', 'libnames']

    def __init__(self):
        self.srcmodule = SourceModule()
        self.libnames = []

    def add_config(self, config):
        self.srcmodule.add_config(config)

    def add_source(self, src):
        self.srcmodule.add_source(src)

    def add_libname(self, libname):
        self.libnames.append(libname)

    def scan_bundled_libs(self, paths):
        for path in paths:
            fname = path.basename
            for libname in self.libnames:
                if fname == libname or fname.startswith(libname + '-'):
                    sys.stderr.write(
                        'found bundled library: {}\n'.format(path.posix))
                    self.srcmodule.prefix_paths(path)
                    return self.srcmodule
