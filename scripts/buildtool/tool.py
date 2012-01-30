from __future__ import with_statement
import os
import buildtool.source as source
import sys
import shutil
import posixpath
import buildtool.git as git
from buildtool.env import Environment
import re

ACTIONS = ['cmake', 'gmake', 'xcode', 'build']
DEFAULT = {
    'Linux': 'gmake',
    'Darwin': 'xcode',
    'Windows': 'cmake',
}

class ToolInvocation(object):
    def __init__(self, tool):
        self._tool = tool
        self.warning = 'This file automatically generated by buildgen.py'

    def _mkdirp(self, path):
        apath = self._tool._rootpath(path)
        if not path or os.path.isdir(apath):
            return
        self._mkdirp(os.path.dirname(path))
        os.mkdir(apath)

    def write_file(self, path, data):
        """Write a file with the given contents to the given path."""
        self._mkdirp(os.path.dirname(path))
        print path
        abspath = self._tool._rootpath(path)
        try:
            with open(abspath, 'w') as f:
                f.write(data)
        except:
            try:
                os.unlink(abspath)
            except OSError:
                pass
            raise

    def get_atoms(self, *atoms, **kw):
        """Get all source files with the given atoms.

        This returns a list of all source files which have at least
        one of the atoms listed.  If None appears in the list, then
        source files with no atoms will also be included.
        """
        try:
            native = kw['native']
        except KeyError:
            native = False
        else:
            del kw['native']
        if kw:
            raise ValueError('unexpected keyword arguments')
        result = []
        for src in self._tool._sources:
            for atom in atoms:
                if atom is None:
                    if not src.atoms:
                        result.append(src.path)
                        break;
                else:
                    if atom in src.atoms:
                        result.append(src.path)
                        break
        if native and os.path.sep != '/':
            sep = os.path.sep
            result = [path.replace('/', sep) for path in result]
        return result

    def all_sources(self, native=False):
        """Get a list of all sources."""
        if native and os.path.sep != '/':
            return [src.path.replace('/', sep)
                    for src in self._tool._sources]
        else:
            return [src.path for src in self._tool._sources]

    @property
    def incldirs(self):
        return self._tool._incldirs

    @property
    def env(self):
        return self._tool.env

    @property
    def opts(self):
        return self._tool.opts

    def _writeversion(self):
        vers = git.describe('.')
        self.write_file(
            'version.c',
            'const char SG_VERSION[] = "%s";\n' % vers)

class Tool(object):
    def __init__(self):
        self._rootdir = None
        self._atoms = set()
        self._sources = []
        self._incldirs = []
        self._props = {}
        self.env = Environment()

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
        self.opts = opts
        actions = []
        for arg in args:
            idx = arg.find('=')
            if idx >= 0:
                var = arg[:idx]
                val = arg[idx+1:]
                self.env[var] = val
            else:
                if arg not in ACTIONS:
                    print >>sys.stderr, 'Error: unknown backend %r' % arg
                    sys.exit(1)
                actions.append(arg)

        if not actions:
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

        self._sources.append(source.Source('version.c', []))
        i = ToolInvocation(self)
        i._writeversion()
        for b in actions:
            m = getattr(__import__('buildtool.' + b), b)
            m.run(i)

    def run(self):
        import optparse
        p = optparse.OptionParser()
        p.add_option('--config', dest='config',
                     help="use configuration (debug, release)",
                     default='release')
        p.add_option('--verbose', dest='verbose',
                     action='store_true', default=False,
                     help="print variables and command lines")
        opts, args = p.parse_args()
        self._run(opts, args)
