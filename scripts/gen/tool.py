from __future__ import with_statement
import os
import gen.source as source
import sys
import shutil
import posixpath
import gen.git as git
from gen.env import Environment
import re

ACTIONS = ['cmake', 'gmake', 'xcode', 'build']
DEFAULT = {
    'Linux': 'gmake',
    'Darwin': 'xcode',
    'Windows': 'cmake',
}

def memoize(fn):
    def decorated(self):
        try:
            memo = self._memo
        except AttributeError:
            memo = {}
            self._memo = memo
        try:
            return memo[fn.__name__]
        except KeyError:
            v = fn(self)
            memo[fn.__name__] = v
            return v
    return decorated

class ToolInvocation(object):
    def __init__(self, tool):
        self._tool = tool
        self.warning = 'This file automatically generated by buildgen.py'

    def _mkdirp(self, path):
        if not path or os.path.isdir(path):
            return
        self._mkdirp(os.path.dirname(path))
        os.mkdir(path)

    def write_file(self, path, data):
        """Write a file with the given contents to the given path."""
        self._mkdirp(os.path.dirname(path))
        print 'FILE', path
        try:
            with open(path, 'w') as f:
                f.write(data)
        except:
            try:
                os.unlink(path)
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

    @property
    def version(self):
        return self._tool.version

    @property
    def sgpath(self):
        return self._tool._sgpath

    @memoize
    def version_file(self):
        path = 'version.c'
        vers = self.version
        self.write_file(
            path,
            'const char SG_VERSION[] = "%s";\n' % vers)
        return path

    def _ftemplate(self, ipath, opath, vars=None):
        import re
        data = open(ipath, 'rb').read()
        def repl(m):
            w = m.group(1)
            try:
                return self.env[w]
            except KeyError:
                pass
            if vars is not None:
                try:
                    v = vars[w]
                    if v is None:
                        return m.group(0)
                    return v
                except KeyError:
                    pass
            print >>sys.stderr, '%s: unknown variable %r' % (ipath, w)
            return m.group(0)
        data = re.sub(r'\${(\w+)}', repl, data)
        self.write_file(opath, data)

    @memoize
    def info_plist(self):
        import plistxml
        path = os.path.join('resources', 'mac', 'Info.plist')
        plist = plistxml.load(open(os.path.join(self.sgpath, path), 'rb').read())
        plist[u'CFBundleIdentifier'] = unicode(self.env['PKG_IDENT'])
        try:
            icons = self.env['EXE_MACICON']
        except KeyError:
            pass
        else:
            plist[u'CFBundleIconFile'] = unicode(os.path.basename(icons))
        self.write_file(path, plistxml.dump(plist))
        return path

    @memoize
    def main_xib(self):
        path = os.path.join('resources', 'mac', 'MainMenu.xib')
        self._ftemplate(os.path.join(self.sgpath, path), path)
        return path

class Tool(object):
    def __init__(self, rootdir):
        rootpath = os.path.abspath(rootdir)
        sgpath = os.path.abspath(__file__)
        for i in xrange(3):
            sgpath = os.path.dirname(sgpath)
        sgrel = []
        p = sgpath
        while p != rootpath:
            np, f = os.path.split(p)
            if np == p:
                print >>sys.stderr, 'root: %s, sglib: %s' % (rootpath, sgpath)
                print >>sys.stderr, 'error: sglib must be below root directory'
                sys.exit(1)
            p = np
            sgrel.insert(0, f)
        sgpath = os.path.join(*sgrel)
        os.chdir(rootpath)

        self._atoms = set()
        self._sources = []
        self._incldirs = []
        self._props = {}
        self.env = Environment()
        self._sgpath = sgpath

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
        for line in open(path, 'r'):
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
                actions = DEFAULT[s]
            except KeyError:
                print >>sys.stderr, 'Error: no action specified\n' \
                    'No default action for %s platform' % s
                sys.exit(1)
            if isinstance(actions, str):
                actions = [actions]

        self.version = git.describe(self, '.')
        i = ToolInvocation(self)
        self._sources.append(source.Source(i.version_file(), []))
        for b in actions:
            print 'ACTION', b
            m = getattr(__import__('gen.' + b), b)
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
        p.add_option('--no-configure', dest='configure',
                     action='store_false', default=True,
                     help="do not run the configure script")
        p.add_option('--dump-env', dest='dump_env',
                     action='store_true', default=False,
                     help='print all environment variables')
        p.add_option('--with-git', dest='git_path',
                     help='use git executable at PATH', metavar='PATH')
        opts, args = p.parse_args()
        self._run(opts, args)
