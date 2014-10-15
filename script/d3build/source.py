import os

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
        self.tags = tuple(tags)

    def __repr__(self):
        return 'SourceFile({!r}, {!r})'.format(self.path, self.tags)

class SourceList(object):
    """A collection of source files."""
    __slots__ = ['sources', '_base']

    def __init__(self, *, path, base=None, sources=None):
        self.sources = []
        dirpath = os.path.dirname(path)
        if base is None:
            self._base = dirpath
        else:
            self._base = _join(dirpath, base)
        if sources is not None:
            self.add(sources)

    def add(self, sources, *, base=None, tags=None):
        """Add sources to the source list.

        The sources are specified as a string, with one file per line.
        """
        if base is None:
            base = self._base
        else:
            base = _join(self._base, base)
        for line in sources.splitlines():
            fields = line.split()
            if not fields:
                continue
            path = _join(base, fields[0])
            self.sources.append(SourceFile(path, fields[1:]))
