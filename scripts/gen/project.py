import os
import gen.source as source
import sys
from gen.env import Environment
from gen.path import Path

class Project(object):
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
        self._props = {}
        self._env = Environment(
            CPPPATH=os.path.join(sgpath, 'src'),
        )
        self._sgpath = sgpath

        self._sources.read_list(
            'sglib', os.path.join(sgpath, 'src/srclist-base.txt'), ())
        self._sources.read_list(
            'sglib', os.path.join(sgpath, 'src/srclist-client.txt'), ('cxx',))

    @property
    def sgpath(self):
        """The path to the sglib library root."""
        return self._sgpath

    @property
    def sourcelist(self):
        """The SourceList object containing this project's sources."""
        return self._sources

    @property
    def env(self):
        """The project environment."""
        return self._env

    def add_sourcelist(self, name, path):
        """Add a list of source files from a list at the given path.

        The file is relative to the source directory, and paths in the
        file are relative to the file's directory.  The name controls
        how the source list appears as a group in IDEs.

        The path to the sourcelist may be specified as a POSIX path or
        using native directory separators.
        """
        self._sources.read_list(name, path.replace('/', os.path.sep), ())

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
