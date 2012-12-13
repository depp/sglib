"""Source files."""
import gen.path

class Source(object):
    """A source file.

    Has a path, a source type, and a set of tags that describe how and
    when the source file is compiled.

    The path is relative to the module root.
    """
    __slots__ = ['path', 'tags', 'sourcetype']

    def __init__(self, path, tags):
        if not isinstance(path, gen.path.Path):
            raise TypeError('path must be Path instance')
        if not isinstance(tags, tuple):
            raise TypeError('tags must be tuple')

        ext = path.ext
        if not ext:
            raise ValueError(
                'source file has no extension, cannot determine type: {}'
                .format(path.posix))
        try:
            stype = gen.path.EXTS[ext]
        except KeyError:
            raise ValueError(
                'source file {} has unknown extension, cannot determine type'
                .format(path.posix))

        self.path = path
        self.tags = tags
        self.sourcetype = stype

    def __repr__(self):
        return 'Source({!r}, {!r})'.format(self.path, self.tags)

    def __cmp__(self, other):
        if not isinstance(other, Source):
            return NotImplemented
        return cmp(self.path, other.path)
