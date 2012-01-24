import os
import buildtool.source as source

class Tool(object):
    def __init__(self):
        self._rootdir = None
        self._srcdir = None
        self._atoms = set()
        self._sources = []

    def rootdir(self, path):
        """Set the root directory."""
        if self._rootdir is not None:
            raise Exception('Already have a root directory')
        os.chdir(path)
        self._rootdir = True

    def _rootpath(self, path):
        """Get a rootdir-relative path."""
        if not self._rootdir:
            raise Exception('Need a root directory')
        return path

    def srcdir(self, path):
        """Set the source directory."""
        if self._srcdir is not None:
            raise Exception('Already have a source directory')
        self._srcdir = self._rootpath(path)

    def _srcpath(self, path):
        """Get a srcdir-relative path."""
        if self._srcdir is None:
            raise Exception('Need a source directory')
        return os.path.join(self._srcdir, path)

    def srclist(self, path, *atoms):
        """Add a list of source files from a list at the given path.

        The file is relative to the source directory, and paths in the
        file are relative to the file's directory.
        """
        base = os.path.dirname(path)
        for line in open(self._srcpath(path), 'r'):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            line = line.split()
            fpath, fatoms = line[0], line[1:]
            fatoms.extend(atoms)
            fpath = os.path.normpath(os.path.join(base, fpath))
            self._atoms.update(fatoms)
            self._sources.append(source.Source(fpath, fatoms))
