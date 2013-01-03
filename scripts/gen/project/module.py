__all__ = ['Executable', 'Module', 'ExternalLibrary', 'BundledLibrary']
from gen.project.source import SourceModule
from gen.error import ConfigError
import gen.util as util
import sys

class SimpleModule(object):
    __slots__ = ['name', 'srcmodule']

    def __init__(self):
        self.name = None
        self.srcmodule = SourceModule()

    def add_source(self, src):
        self.srcmodule.add_source(src)

    def add_config(self, config):
        self.srcmodule.add_config(config)

    def module_deps(self):
        return self.srcmodule.module_deps()

    def enable_flags(self):
        return self.srcmodule.enable_flags()

    def sources(self):
        return iter(self.srcmodule.sources)

    def configs(self):
        yield self.srcmodule.config

class Executable(SimpleModule):
    """An executable target."""

    __slots__ = SimpleModule.__slots__ + [
        'filename', 'exe_icon', 'apple_category', 'config'
    ]

    def __init__(self):
        super(Executable, self).__init__()
        self.filename = {}
        self.exe_icon = []
        self.apple_category = None

    def validate(self, enable):
        errors = []
        if self.name is None:
            errors.append(ConfigError('executable has no name'))
        else:
            try:
                util.make_filenames(self.filename, self.name)
            except ConfigError as ex:
                errors.append(ex)
        try:
            self.srcmodule.validate(enable)
        except ConfigError as ex:
            errors.append(ex)
        if errors:
            raise ConfigError('error in executable', suberrors=errors)

class Module(SimpleModule):
    """A simple module, consisting of project source code."""

    __slots__ = SimpleModule.__slots__

    def __init__(self):
        super(Module, self).__init__()

    def scan_bundled_libs(self, paths):
        pass

    def validate(self, enable):
        errors = []
        if self.name is None:
            errors.append(ConfigError('module has no name'))
        try:
            self.srcmodule.validate(enable)
        except ConfigError as ex:
            errors.append(ex)
        if errors:
            raise ConfigError('error in module', suberrors=errors)

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

    def enable_flags(self):
        flags = set()
        if self.srcmodule is not None:
            flags.update(self.srcmodule.enable_flags())
        for source in self.libsources:
            flags.update(source.enable_flags())
        return flags

    def validate(self, enable):
        errors = []
        if self.name is None:
            errors.append(ConfigError('module has no name'))
        objs = []
        if self.srcmodule is not None:
            objs.append(self.srcmodule)
        objs.extend(self.libsources)
        for obj in objs:
            try:
                obj.validate(enable)
            except ConfigError as ex:
                errors.append(ex)
        if errors:
            raise ConfigError('error in external library', suberrors=errors)

    def sources(self):
        if self.srcmodule:
            return iter(self.srcmodule.sources)
        return ()

    def configs(self):
        for source in self.libsources:
            yield source
        if self.srcmodule is not None:
            yield self.srcmodule.config

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
