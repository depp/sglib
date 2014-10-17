import os

_SRCTYPE_EXTS = {
    'c': 'c',
    'c++': 'cpp cp cxx',
    'h': 'h',
    'h++': 'hpp hxx',
    'objc': 'm',
    'objc++': 'mm',
    'vcxproj': 'vcxproj',
}

EXT_SRCTYPE = {
    '.' + ext: type
    for type, exts in _SRCTYPE_EXTS.items()
    for ext in exts.split()
}

SRCTYPE_EXT = {
    type: '.' + exts.split()[0]
    for type, exts in _SRCTYPE_EXTS.items()
}

del _SRCTYPE_EXTS

def _join(base, path):
    for part in path.split('/'):
        if part == '..':
            base = os.path.dirname(base)
        elif part == '.' or not part:
            pass
        else:
            base = os.path.join(base, part)
    return base

class SourceFile(object):
    """A record of an individual source file."""
    __slots__ = ['path', 'tags']

    def __init__(self, path, tags):
        if not os.path.isabs(path):
            raise ValueError('path must be absolute')
        self.path = path
        self.tags = tuple(sorted(tags))

    def __repr__(self):
        return 'SourceFile({!r}, {!r})'.format(self.path, self.tags)

class SourceList(object):
    """A collection of source files."""
    __slots__ = ['sources', 'tags', '_path']

    def __init__(self, *, base, path=None, sources=None):
        self.sources = []
        self.tags = set()
        dirpath = os.path.dirname(base)
        if path is None:
            self._path = dirpath
        else:
            self._path = _join(dirpath, path)
        if sources is not None:
            self.add(sources)

    def add(self, sources, *, path=None, tags=None):
        """Add sources to the source list.

        The sources are specified as a string, with one file per line.
        """
        tags = set(tags) if tags else set()
        if path is None:
            path = self._path
        else:
            path = _join(self._path, path)
        for line in sources.splitlines():
            fields = line.split()
            if not fields:
                continue
            path = _join(path, fields[0])
            srctags = set(tags)
            for tag in fields[1:]:
                if tag.startswith('!'):
                    tag = tag[1:]
                    srctags.discard(tag)
                else:
                    srctags.add(tag)
                self.tags.add(tag)
            self.sources.append(SourceFile(path, srctags))
