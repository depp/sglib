import posixpath
import re
import os

# http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
PATH_SPECIAL_NAME = set(
    'CON PRN AUX NUL COM1 COM2 COM3 COM4 COM5 COM6 COM7 COM8 '
    'COM9 LPT1 LPT2 LPT3 LPT4 LPT5 LPT6 LPT7 LPT8 LPT9'.split())
PATH_SPECIAL = re.compile('[\x00-\x1f<>:"/\\|?*]')

class Path(object):
    __slots__ = ['path']

    def __init__(self, *args):
        parts = []
        is_dir = False
        for arg in args:
            if isinstance(arg, Path):
                if arg.path == '.':
                    continue
                parts.extend(arg.path.split('/'))
                if not parts[-1]:
                    parts.pop()
                    is_dir = True
                continue
            if arg.startswith('/'):
                raise ValueError(
                    'absolute paths are not allowed: {!r}'.format(path))
            for part in arg.split('/'):
                if not part:
                    is_dir = True
                    continue
                is_dir = False
                if part == '..':
                    if not parts:
                        raise ValueError(
                            'path goes above root: {!r}'.format(arg))
                    parts.pop()
                    continue
                if part == '.':
                    continue
                m = PATH_SPECIAL.search(part)
                if m:
                    raise ValueError('invalid path: {!r}, '
                                     'contains special character {!r}'
                                     .format(arg, m.group(0)))
                if part.endswith('.') or part.endswith(' '):
                    raise ValueError('invalid path component: {!r}'
                                     .format(part))
                i = part.find('.')
                if i >= 0:
                    base = part[:i]
                else:
                    base = part
                if base.upper() in PATH_SPECIAL_NAME:
                    raise ValueError(
                        'invalid path commponent: {!r}'.format(part))
                parts.append(part)
        result = '/'.join(parts)
        if not result:
            result = '.'
        if is_dir:
            result += '/'
        self.path = result

    def dirname(self):
        i = self.path.rfind('/')
        p = Path.__new__(Path)
        if i >= 0:
            p.path = self.path[:i]
        else:
            p.path = '.'
        return p

    def to_string(self, os):
        if os == 'windows':
            return self.path.replace('/', '\\')
        else:
            return self.path

    if os.path.sep != '/':
        def native(self):
            return self.path.replace('/', os.path.sep)
    else:
        def native(self):
            return self.path

    def __str__(self):
        return self.path

    def __repr__(self):
        return 'Path({!r})'.format(self.path)

    def __eq__(self, other):
        if isinstance(other, Path):
            return self.path == other.path
        return NotImplemented

    def __ne__(self, other):
        if isinstance(other, Path):
            return self.path != other.path
        return NotImplemented

    def __lt__(self, other):
        if isinstance(other, Path):
            return self.path < other.path
        return NotImplemented

    def __le__(self, other):
        if isinstance(other, Path):
            return self.path <= other.path
        return NotImplemented

    def __gt__(self, other):
        if isinstance(other, Path):
            return self.path > other.path
        return NotImplemented

    def __ge__(self, other):
        if isinstance(other, Path):
            return self.path >= other.path
        return NotImplemented

    def __hash__(self):
        return hash(self.path)
