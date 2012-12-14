"""Source files."""
import gen.path

class Source(object):
    """A source file.

    Has a path, a source type, and a set of enable flags and module
    dependencies that describe how and when the source file is
    compiled.

    The path is always relative to some root path.
    """
    __slots__ = ['path', 'enable', 'module']

    def __init__(self, path, enable, module):
        self.path = path
        self.enable = tuple(enable)
        self.module = tuple(module)

    def __repr__(self):
        return 'Source({!r}, {!r})'.format(self.path, self.tags)

    def __cmp__(self, other):
        if not isinstance(other, Source):
            return NotImplemented
        return cmp(self.path, other.path)

    @property
    def sourcetype(self):
        ext = self.path.ext
        if not ext:
            raise ValueError(
                'source file has no extension, cannot determine type: {}'
                .format(self.path.posix))
        try:
            return gen.path.EXTS[ext]
        except KeyError:
            raise ValueError(
                'source file {} has unknown extension, cannot determine type'
                .format(self.path.posix))

        
