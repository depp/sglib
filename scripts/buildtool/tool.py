import os

class Tool(object):
    def __init__(self):
        self._rootdir = None
        self._srcdir = None

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

    def srclist(self, path):
        """Add a list of source files from a list at the given path.

        The file is relative to the source directory, and paths in the
        file are relative to the file's directory.
        """
        print self._srcpath(path)
