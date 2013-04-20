"""Cross-platform path utility functions.

Paths are stored as strings using POSIX conventions, but they are
verified to make sure they won't cause trouble on Windows.
"""
import re
import os
import urllib.parse

# http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
PATH_SPECIAL_NAME = set(
    'CON PRN AUX NUL COM1 COM2 COM3 COM4 COM5 COM6 COM7 COM8 '
    'COM9 LPT1 LPT2 LPT3 LPT4 LPT5 LPT6 LPT7 LPT8 LPT9'.split())
PATH_SPECIAL = re.compile('[\x00-\x1f<>:"/\\|?*]')

def splitext(path):
    """Split a path into base and extension."""
    sep = path.rfind('/') + 1
    dot = path.rfind('.', sep)
    if dot >= 0 and any(path[i] != '.' for i in range(sep, dot)):
        return path[:dot], path[dot:]
    return path, ''

def split_native(path):
    """Split a native path into components.

    Components are converted to POSIX convention.  Raises an error if
    the path contains illegal components, or if the path is not
    relative.
    """
    drive, dpath = os.path.splitdrive(path)
    if drive or os.path.isabs(dpath):
        raise ValueError('not a relative path')
    parts = []
    while dpath:
        dpath, part = os.path.split(dpath)
        if not part:
            continue
        if part == os.path.pardir:
            parts.append('..')
            continue
        if part == os.path.curdir:
            parts.append('.')
            continue
        m = PATH_SPECIAL.search(part)
        if m:
            raise ValueError(
                'path contains special character: {!r}'
                .format(m.group(0), part))
        if part.endswith('.') or part.endswith(' '):
            raise ValueError(
                'invalid path component: {!r}'.format(part))
        i = part.find('.')
        base = part[:i] if i >= 0 else part
        if base.upper() in PATH_SPECIAL_NAME:
            raise ValueError(
                'invalid path component: {!r}'.format(part))
        parts.append(part)
    parts.reverse()
    return parts

class Path(object):
    """A sanitized path, relative to a specific root directory."""
    __slots__ = ['path', 'base']

    def __init__(self, path, base):
        if base not in ('srcdir', 'builddir'):
            raise ValueError('base must be srcdir or buliddir')
        self.path = path
        self.base = base

    def split(self):
        """Split a path into dirname and basename.

        This will remove extra slashes at the split, like os.path.split().
        """
        path = self.path
        sep = path.rfind('/') + 1
        dirn = path[:sep]
        basen = path[sep:]
        if dirn and dirn == '/' * len(dirn):
            dirn = '/'
        else:
            dirn = dirn.rstrip('/')
        return Path(dirn, self.base), basen

    def dirname(self):
        return self.split()[0]

    def basename(self):
        return self.split()[1]

    def splitext(self):
        """Split a path into base and extension."""
        path = self.path
        sep = path.rfind('/') + 1
        dot = path.rfind('.', sep)
        if dot >= 0 and any(path[i] != '.' for i in range(sep, dot)):
            return Path(path[:dot], self.base), path[dot:]
        return self, ''

    def withext(self, ext):
        """Replace the path's extension with a different one."""
        base, oldext = splitext(self.path)
        if base.endswith('/'):
            raise ValueError('cannot replace extension on directory')
        return Path(base + ext, self.base)

    def addext(self, ext):
        """Add an extension to this path."""
        if self.path.endswith('/'):
            raise ValueError('cannot add extension to directory')
        return Path(self.path + ext, self.base)

    def join(self, *paths):
        """Create a path by joining strings onto a path."""
        parts = self.path[1:].split('/')
        if not parts[-1]:
            parts.pop()
            is_dir = True
        else:
            is_dir = False
        for path in paths:
            if path.startswith('/'):
                parts = []
                is_dir = True
            for part in path.split('/'):
                is_dir = False
                if not part or part == '.':
                    is_dir = True
                    continue
                if part == '..':
                    if not parts:
                        raise ValueError(
                            'path not contained in ${{{}}}'
                            .format(self.base))
                    parts.pop()
                    is_dir = True
                    continue
                m = PATH_SPECIAL.search(part)
                if m:
                    raise ValueError(
                        'path contains special character: {!r}'
                        .format(m.group(0)))
                if part.endswith('.') or part.endswith(' '):
                    raise ValueError(
                        'invalid path component: {!r}'.format(part))
                i = part.find('.')
                base = part[:i] if i >= 0 else part
                if base.upper() in PATH_SPECIAL_NAME:
                    raise ValueError(
                        'invalid path component: {!r}'.format(part))
                parts.append(part)
        if parts and is_dir:
            parts.append('')
        parts.insert(0, '')
        return Path('/'.join(parts), self.base)

    def join1(self, filename, ext=''):
        """Create a path by appending a single filename."""
        if not filename:
            raise ValueError('empty filename')
        if filename.startswith('.'):
            raise ValueError('filename starts with period')
        m = PATH_SPECIAL.search(filename)
        if m:
            raise ValueError(
                'filename contains special character: {!r}'
                .format(m.group(0)))
        if filename.endswith('.') or filename.endswith(' '):
            raise ValueError('invalid filename: {!r}'.format(filename))
        i = filename.find('.')
        base = filename[:i] if i >= 0 else filename
        if base.upper() in PATH_SPECIAL_NAME:
            raise ValueError(
                'invalid filename: {!r}'.format(filename))
        if self.path.endswith('/'):
            return Path('{}{}{}'.format(self.path, filename, ext), self.base)
        return Path('{}/{}{}'.format(self.path, filename, ext), self.base)

    def prefix(self, path):
        """Add a prefix to this path."""
        return path.join(self.to_posix())

    def to_posix(self):
        return self.path[1:]

    def to_windows(self):
        return self.path[1:].replace('/', '\\')

    def __str__(self):
        return '${{{}}}{}'.format(self.base, self.path)

    def __repr__(self):
        return 'Path({!r}, {!r})'.format(self.path, self.base)

    def __eq__(self, other):
        if not isinstance(other, Path):
            return NotImplemented
        return self.path == other.path and self.base == other.base

    def __ne__(self, other):
        if not isinstance(other, Path):
            return NotImplemented
        return self.path != other.path or self.base != other.base

    def __hash__(self):
        return hash((self.base, self.path))

    def __lt__(self, other):
        if not isinstance(other, Path):
            return NotImplemented
        return (self.base, self.path) < (other.base, other.path)

    def __le__(self, other):
        if not isinstance(other, Path):
            return NotImplemented
        return (self.base, self.path) <= (other.base, other.path)

    def __gt__(self, other):
        if not isinstance(other, Path):
            return NotImplemented
        return (self.base, self.path) > (other.base, other.path)

    def __ge__(self, other):
        if not isinstance(other, Path):
            return NotImplemented
        return (self.base, self.path) >= (other.base, other.path)

class Href(object):
    __slots__ = ['path', 'frag']

    def __init__(self, path, frag):
        self.path = path
        self.frag = frag

    @classmethod
    def parse(class_, text, base, *, search=None):
        """Parse an href, normalizing the path.

        If a search function is specified, it is used to resolve paths
        that are relative but do not start with '.' or '..'.  The
        function should take the path string to resolve as an argument
        and return the resolved Path object.
        """
        url = urllib.parse.urlsplit(text)
        if url.scheme or url.netloc or url.query:
            raise ValueError('invalid reference')
        path = url.path
        if path:
            hpath = None
            if search is not None:
                i = path.find('/')
                if i >= 0:
                    firstpart = path[:i]
                else:
                    firstpart = path
                if firstpart not in ('', '.', '..'):
                    hpath = search(path)
            if hpath is None:
                basedir, basef = base.split()
                if basef:
                    base = basedir
                hpath = base.join(urllib.parse.unquote(path))
        else:
            hpath = base
        frag = url.fragment or None
        return class_(hpath, frag)

    def __str__(self):
        path = (urllib.parse.quote(self.path.path)
                if self.path is not None else '')
        frag = (urllib.parse.quote(self.frag, safe='/?')
                if self.frag is not None else '')
        if frag:
            return '{}#{}'.format(path, frag)
        return path

    def __repr__(self):
        return 'Href({!r}, {!r})'.format(self.path, self.frag)

    def __eq__(self, other):
        if not isinstance(other, Href):
            return NotImplemented
        return self.path == other.path and self.frag == other.frag

    def __ne__(self, other):
        if not isinstance(other, Href):
            return NotImplemented
        return self.path != other.path or self.frag != other.frag

    def __hash__(self):
        return hash((self.path, self.frag))

    def __lt__(self, other):
        if not isinstance(other, Href):
            return NotImplemented
        if self.path is None:
            if other.path is not None:
                return True
        else:
            if other.path is None:
                return False
            else:
                if self.path < other.path:
                    return True
                elif self.path > other.path:
                    return False
        if self.frag is None:
            if other.frag is not None:
                return True
        else:
            if other.frag is None:
                return False
            else:
                if self.frag < other.frag:
                    return True
                elif self.frag > other.frag:
                    return False
        return False

