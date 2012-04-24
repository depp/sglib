import os
import gen.source as source
import sys
import gen.smartdict as smartdict
from gen.env import Environment
from gen.path import Path
from gen.info import ProjectInfo, ExecInfo
from gen.version import version_cmp
__all__ = ['Module', 'Executable', 'Project', 'Path']

class Module(object):
    """Optional project module.

    A module is identified by an atom.  Sources tagged with that atom
    will be skipped unless the corresponding module is enabled.
    Enabling a module will also add header paths to the target, and
    enable all modules that the module depends on.
    """
    __slots__ = ['_atom', '_desc', '_ipath', '_defs', '_reqs']

    def __init__(self, atom, desc, ipath=(), reqs=(), defs=()):
        self._atom = smartdict.AtomKey.check(atom)
        self._desc = desc
        self._ipath = smartdict.PathListKey.check(ipath)
        self._defs = smartdict.CDefsKey.check(defs)
        self._reqs = smartdict.AtomListKey.check(reqs)

    @property
    def atom(self):
        """The atom that identifies this module."""
        return self._atom

    @property
    def desc(self):
        """A human-readable description of this module."""
        return self._desc

    @property
    def ipath(self):
        """Header search paths for this module."""
        return self._ipath

    @property
    def defs(self):
        """C preprocessor definitions."""
        return self._defs

    @property
    def reqs(self):
        """Atoms identifying modules which this module depends on."""
        return self._reqs

class Executable(Module):
    """Module that produces an executable."""
    __slots__ = ['_atom', '_desc', '_ipath', '_reqs', '_info']

    def __init__(self, atom, desc, ipath=(), reqs=(), **kw):
        Module.__init__(self, atom, desc, ipath, reqs)
        self._info = ExecInfo(**kw)

    @property
    def info(self):
        """The executable info dictionary."""
        return self._info

class Project(object):
    """Top level project.

    A project consists of a project info dictionary, a list of source
    files, and a list of modules.
    """
    __slots__ = ['_sources', '_info', '_sgpath', '_modules',
                 '_libavail']

    def __init__(self, rootdir):
        rootpath = os.path.abspath(rootdir)
        sgpath = os.path.abspath(__file__)
        for i in xrange(3):
            sgpath = os.path.dirname(sgpath)
        # Find common parent directory
        sgrel = []
        p = sgpath
        r = rootpath + os.path.sep
        while not r.startswith(p + os.path.sep):
            np, c = os.path.split(p)
            if np == p:
                print >>sys.stderr, 'root: %s, sglib: %s' % (rootpath, sgpath)
                print >>sys.stderr, 'error: no common directory'
                sys.exit(1)
            p = np
            sgrel.append(c);
        r = rootpath
        while r != p:
            np, c = os.path.split(r)
            assert np != r
            r = np
            sgrel.append('..')
        sgrel.reverse()
        sgpath = os.path.join(*sgrel)
        os.chdir(rootpath)

        self._sources = source.SourceList()
        self._info = ProjectInfo()
        self._sgpath = Path(sgpath.replace(os.path.sep, '/'))
        self._modules = {}

        sgincpath = Path(self._sgpath, 'src')
        self._sources.read_list(
            'SGLib', os.path.join(sgpath, 'src/srclist-base.txt'),
            ('SGLIB',))
        self._sources.read_list(
            'SGLib', os.path.join(sgpath, 'src/srclist-client.txt'),
            ('SGLIBXX',))
        self.add_module(
            Module('SGLIB', 'SGLib C client code',
                   ipath=sgincpath))
        self.add_module(
            Module('SGLIBXX', 'SGLib C++ client code',
                   ipath=sgincpath, reqs='SGLIB'))

    @property
    def modules(self):
        """All modules in the project."""
        return self._modules

    @property
    def sgpath(self):
        """The path to the sglib library root."""
        return self._sgpath

    @property
    def sourcelist(self):
        """The SourceList object containing this project's sources."""
        return self._sources

    @property
    def info(self):
        """The project info dictionary."""
        return self._info

    def add_sourcelist(self, name, path, *atoms):
        """Add a list of source files from a list at the given path.

        The file is relative to the source directory, and paths in the
        file are relative to the file's directory.  The name controls
        how the source list appears as a group in IDEs.

        The path to the sourcelist may be specified as a POSIX path or
        using native directory separators.
        """
        self._sources.read_list(name, path.replace('/', os.path.sep), atoms)

    def add_sourcelist_str(self, name, gpath, text, *atoms):
        """Add a list of source files to the given group.

        Like add_sourcelist, except with a string instead of a filename.
        """
        self._sources.read_list_str(name, Path(gpath), text, atoms)

    def includepath(self, *paths):
        """Search the given directories for header files."""
        for path in paths:
            if not os.path.isdir(path):
                print >>sys.stderr, 'Warning: not a directory: %r' % path
        raise Exception('not implemented')

    def run(self):
        """Run the command line tool for building the project."""
        import gen.run
        gen.run.run(self)

    def add_module(self, module):
        if not isinstance(module, Module):
            raise TypeError('module must be Module')
        if module.atom in self._modules:
            raise Exception('duplicate module added: %s' % (module.atom,))
        self._modules[module.atom] = module

    def targets(self):
        """Iterate over all target modules."""
        for m in self._modules.itervalues():
            if isinstance(m, Executable):
                yield m

    def _find_library(self, name):
        """Get the path to the given library."""
        libpath = Path(self._sgpath, 'lib')
        try:
            libavail = self._libavail
        except AttributeError:
            libavail = {}
            for fname in os.listdir(libpath.native):
                i = fname.find('-')
                if i >= 0:
                    libname = fname[:i]
                    libver = fname[i+1:]
                else:
                    libname = fname
                    libver = ''
                try:
                    vers = libavail[libname]
                except KeyError:
                    vers = []
                    libavail[libname] = vers
                vers.append((libver, fname))
            self._libavail = libavail
        try:
            libvers = libavail[name]
        except KeyError:
            print >>sys.stderr, 'error: library not found: %s' % (name,)
            print >>sys.stderr, 'please extract library code in %s' % \
                (libpath.native,)
            sys.exit(1)
        libvers.sort(cmp=lambda x, y: version_cmp(x[0], y[0]))
        libver, fname = libvers[-1]
        print >>sys.stderr, 'using %s version %s' % (name, libver)
        return Path(libpath, fname)
