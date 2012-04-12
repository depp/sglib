"""Path manipulation utilities."""
import os
import re
import posixpath

_VALID_PATH = re.compile(r'^[A-Za-z0-9](?:[- _.A-Za-z0-9]*[A-Za-z0-9])$')

C_EXTS = ['.c']
CXX_EXTS = ['.cpp', '.cxx', '.cp']
H_EXTS = ['.h']
HXX_EXTS = ['.hpp', '.hxx']
OBJC_EXTS = ['.m']
OBJCXX_EXTS = ['.mm']

TYPES = {'c': C_EXTS, 'h': H_EXTS, 'cxx': CXX_EXTS,
         'hxx': HXX_EXTS, 'm': OBJC_EXTS, 'mm': OBJCXX_EXTS}
def _compute_exts():
    exts = {}
    for what, texts in TYPES.iteritems():
        for ext in texts:
            exts[ext] = what
    return exts
EXTS = _compute_exts()

def filterexts(paths, exts):
    """Return a list of the paths whose extensions are in exts."""
    return [x for x in paths if os.path.splitext(x)[1] in exts]

def c_sources(paths):
    """Return a list of the paths which are C sources."""
    return filterexts(paths, C_EXTS)

def cxx_sources(paths):
    """Return a list of the paths which are C++ sources."""
    return filterexts(paths, CXX_EXTS)

def sources(paths):
    """Return a list of the paths which are sources (not headers)."""
    return filterexts(paths, C_EXTS + CXX_EXTS)

def withext(paths, ext):
    """Return paths with their extensions changed to ext."""
    return [os.path.splitext(x)[0] + ext for x in paths]

class Path(object):
    """Representation of a file path.

    The path must be a relative path, and it must also be 'safe'.
    Each component must start and end with an alphanumeric character,
    and may only contain alphanumeric characters and '-_.'.  A
    sequence of components at the beginning may be '..'.  Paths must
    be specified using the POSIX directory separator.  Any number of
    paths can be specified as constructor arguments, the paths will be
    concatenated as if by the posixpath.join function.

    If the path is empty or refers to the current directory, it will
    be equal to '.'.  In all other cases, no component of the path
    will be '.'.

    Paths can be joined using the / operator, or by passing multiple
    paths to the path constructor.
    """
    __slots__ = ['_p']
    def __init__(self, *paths):
        pp = []
        valid = _VALID_PATH.match
        for path in paths:
            if isinstance(path, Path):
                path = path._p
            if path.startswith('/'):
                raise ValueError("Invalid path %r: absolute" % (path,))
            for p in path.split('/'):
                if p == '.' or p == '':
                    pass
                elif p == '..':
                    isdir = True
                    if not pp or pp[-1] == '..':
                        pp.append('..')
                    else:
                        pp.pop()
                else:
                    if valid(p) and '  ' not in p:
                        pp.append(p)
                        isdir = False
                    else:
                        raise ValueError(
                            "Invalid path %r: bad component %r" % (path, p))
        if not pp:
            self._p = '.'
        else:
            self._p = '/'.join(pp)
    @property
    def posix(self):
        """Get the POSIX version of this path."""
        return self._p
    @property
    def native(self):
        """Get the native version of this path."""
        return self._p.replace('/', os.path.sep)
    @property
    def windows(self):
        """Get the Windows verrsion of this path."""
        return self._p.replace('/', '\\')
    @property
    def ext(self):
        """Get the path extension.

        As with os.path.splitext, this will include the period in the
        result, unless the path has no extension."""
        return posixpath.splitext(self._p)[1]
    def __repr__(self):
        return 'Path(%r)' % (self._p,)
    def __div__(self, other):
        return Path(self._p, other._p)
    def __eq__(self, other):
        if not isinstance(other, Path):
            raise TypeError('can only compare against other Path objects')
        return self._p == other._p
    def __ne__(self, other):
        return not (self == other)
    def __hash__(self):
        return hash(self._p)
    def withext(self, ext):
        """Return a copy of the path with the extension changed.

        If the path has no extension, then the new extension is added.
        If the extension is empty, the result will have the extension
        removed.  If the extension is not empty, it should start with
        a period.
        """
        if not ext.startswith('.') and ext:
            raise ValueError(
                'extension should be empty or start with a period')
        if self._p == '.':
            raise ValueError('cannot set extension on empty path')
        return Path(posixpath.splitext(self._p)[0] + ext)
    def addext(self, ext):
        """Return a copy of the path with an extension added.

        The extension should start with a period.
        """
        if not ext.startswith('.'):
            raise ValueError('extension should start with a period')
        if self._p == '.':
            raise ValueError('cannot add extension to empty path')
        return Path(self._p + ext)
    @property
    def basename(self):
        """Get the base name, the last component of the path."""
        return posixpath.basename(self._p)
