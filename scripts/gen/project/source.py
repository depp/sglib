__all__ = ['Source', 'SourceModule']
from gen.path import Path
from gen.project.config import ConfigSet

class Source(object):
    """A source file.

    Has a path, a source type, and a set of enable flags and module
    dependencies that describe how and when the source file is
    compiled.

    The path is always relative to some root path.
    """
    __slots__ = ['path', 'enable', 'module']

    def __init__(self, path, enable, module):
        self.path = Path(path)
        self.enable = tuple(enable)
        self.module = tuple(module)

    def __repr__(self):
        return ('Source({!r}, {!r}, {!r})'
                .format(self.path, self.enable, self.module))

    def __cmp__(self, other):
        if not isinstance(other, Source):
            return NotImplemented
        return cmp(self.path, other.path)

    @property
    def sourcetype(self):
        ext = self.path.ext
        if not ext:
            raise ValueError(
                'source file has no extension, cannot determine type: {}'
                .format(self.path.posix))
        try:
            return gen.path.EXTS[ext]
        except KeyError:
            raise ValueError(
                'source file {} has unknown extension, cannot determine type'
                .format(self.path.posix))

    def prefix_paths(self, prefix):
        """Modify the source by prepending its path with a prefix."""
        self.path = Path(prefix, self.path)

class SourceModule(object):
    """A source module.

    A source module is a group of source files.  Source modules have
    configuration settings and can pull in dependencies.  This class
    should not be subclassed.

    sources: The source code in the module.

    config: The configuration settings for this module.
    """

    __slots__ = [
        'sources',
        'config',
    ]

    def __init__(self):
        self.sources = []
        self.config = ConfigSet()

    def add_source(self, src):
        self.sources.append(src)

    def add_config(self, config):
        self.config.add_config(config)

    def module_root(self):
        """Get the directory containing all module sources.

        This will throw an exception for modules containing no sources
        (e.g., external libraries).
        """
        # FIXME: use of this method is sign of a HACK
        return common_ancestor(src.path for src in self.sources)

    def module_deps(self):
        """Return a set of all modules this module may depend on."""
        modules = set()
        for c in self.config.all_configs():
            try:
                cmodules = c.modules
            except AttributeError:
                pass
            else:
                modules.update(cmodules)
        return modules

    def validate(self, enable):
        """Validate the source module structure.

        The enable parameter should be a set of all enable flags in
        the project.
        """

        modules = self.module_deps()
        unsat_modules = {}
        unsat_enable = {}
        for src in self.sources:
            for modid in src.modules:
                if modid not in modules:
                    try:
                        unsat = unsat_modules[modid]
                    except KeyError:
                        unsat = []
                        unsat_modules[modid] = unsat
                    unsat.append(src.path.posix)
            for flagid in src.enable:
                if flagid not in enable:
                    try:
                        unsat = unsat_enable[flagid]
                    except KeyError:
                        unsat = []
                        unsat_enable = unsat
                    unsat.append(src.path.posix)

        if unsat_modules or unsat_enable:
            fp = io.StringIO()
            if unsat_modules:
                fp.write('modules required by source files must also '
                         'be required by the module')
            for k, v in unsat_modules.items():
                fp.write('invalid module requirement: {}\n' .format(k))
                fp.write('    required by: {}\n'
                         .format(', '.join(sorted(set(v)))))
            for k, v in unsat_enable.items():
                fp.write('unknown enable flag: {}\n'.format(k))
                fp.write('    required by: {}\n'
                         .format(', '.join(sorted(set(v)))))
            raise ConfigError('source module contains errors', fp.getvalue())

    def prefix_paths(self, prefix):
        """Modify the module by prepending all paths with a prefix."""
        for src in self.sources:
            src.prefix_paths(prefix)
        self.config.prefix_paths(prefix)
