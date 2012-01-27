from __future__ import with_statement
import os
import buildtool.source as source
import sys
import shutil
import posixpath

ACTIONS = ['cmake', 'gmake', 'xcode']
DEFAULT = {
    'Linux': 'gmake',
    'Darwin': 'xcode',
    'Windows': 'cmake',
}

class Tool(object):
    def __init__(self):
        self._rootdir = None
        self._atoms = set()
        self._sources = []
        self._incldirs = []
        self._warning = 'This file automatically generated by buildgen.py'

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

    def srclist(self, path, *atoms):
        """Add a list of source files from a list at the given path.

        The file is relative to the source directory, and paths in the
        file are relative to the file's directory.
        """
        if os.path.sep != '/':
            base = path.replace(os.path.sep, '/')
        else:
            base = path
        base = posixpath.dirname(base)
        for line in open(self._rootpath(path), 'r'):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            line = line.split()
            fpath, fatoms = line[0], line[1:]
            fatoms.extend(atoms)
            fpath = posixpath.join(base, fpath)
            self._atoms.update(fatoms)
            self._sources.append(source.Source(fpath, fatoms))

    def includepath(self, *paths):
        """Search the given directories for header files."""
        for path in paths:
            if not os.path.isdir(path):
                print >>sys.stderr, 'Warning: not a directory: %r' % path
        self._incldirs.extend(paths)

    def _run(self, opts, args):
        actions = []
        if not args:
            import platform
            s = platform.system()
            try:
                args = DEFAULT[s]
            except KeyError:
                print >>sys.stderr, 'Error: no action specified\n' \
                    'No default action for %s platform' % s
                sys.exit(1)
            if isinstance(args, str):
                args = (args,)
        for arg in args:
            if arg not in ACTIONS:
                print >>sys.stderr, 'Error: unknown backend %r' % arg
                sys.exit(1)
            actions.append(arg)
        for b in actions:
            m = getattr(__import__('buildtool.' + b), b)
            m.run(self)

    def run(self):
        import optparse
        p = optparse.OptionParser()
        opts, args = p.parse_args()
        self._run(opts, args)

    def _mkdirp(self, path):
        apath = self._rootpath(path)
        if not path or os.path.isdir(apath):
            return
        self._mkdirp(os.path.dirname(path))
        os.mkdir(apath)

    def _write_file(self, path, data):
        self._mkdirp(os.path.dirname(path))
        print path
        abspath = self._rootpath(path)
        try:
            with open(abspath, 'w') as f:
                f.write(data)
        except:
            try:
                os.unlink(abspath)
            except OSError:
                pass
            raise

    def _get_atoms(self, *atoms):
        result = []
        for src in self._sources:
            for atom in atoms:
                if atom is None:
                    if not src.atoms:
                        result.append(src.path)
                        break;
                else:
                    if atom in src.atoms:
                        result.append(src.path)
                        break
        return result
