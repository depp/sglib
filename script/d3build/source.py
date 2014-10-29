# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import os
from .error import UserError

_SRCTYPE_EXTS = {
    'c': 'c',
    'c++': 'cpp cp cxx',
    'h': 'h',
    'h++': 'hpp hxx',
    'objc': 'm',
    'objc++': 'mm',
    'rc': 'rc',
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
    orig_base = base
    for part in path.split('/'):
        if part == '..':
            if base == '.' or not base:
                raise UserError('path outside root: base={}, path={}'
                                .format(orig_base, path))
            base = os.path.dirname(base) or '.'
        elif part == '.' or not part:
            pass
        elif base == '.':
            base = part
        else:
            base = os.path.join(base, part)
    return base

def _base(base, path):
    dirpath = os.path.dirname(os.path.relpath(base)) or '.'
    if path is None:
        return dirpath
    return _join(dirpath, path)

class TagSourceFile(object):
    """A record of an individual source file."""
    __slots__ = ['path', 'tags', 'sourcetype']

    def __init__(self, path, tags, sourcetype):
        self.path = path
        self.tags = tuple(tags)
        self.sourcetype = sourcetype

    def __repr__(self):
        return ('TagSourceFile({!r}, {!r}, {!r})'
                .format(self.path, self.tags, self.sourcetype))

class SourceFile(object):
    """An individual source file with its build variables."""
    __slots__ = ['path', 'varsets', 'sourcetype', 'external']

    def __init__(self, path, varsets, sourcetype, external):
        if os.path.isabs(path):
            raise ValueError('path must be relative to the root')
        self.path = path
        self.varsets = varsets
        self.sourcetype = sourcetype
        self.external = bool(external)

    def __repr__(self):
        return ('SourceFile({!r}, {!r}, {!r}, {!r})'
                .format(self.path, self.varsets,
                        self.sourcetype, self.external))

class SourceList(object):
    """A collection of source files."""
    __slots__ = ['sources', 'tags', '_path']

    def __init__(self, *, base='.', path=None, sources=None):
        self.sources = []
        self.tags = set()
        self._path = _base(base, path)
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
            filepath = _join(path, fields[0])
            srctags = set(tags)
            for tag in fields[1:]:
                if tag.startswith('!'):
                    tag = tag[1:]
                    srctags.discard(tag)
                else:
                    srctags.add(tag)
                self.tags.add(tag)
            ext = os.path.splitext(filepath)[1]
            try:
                sourcetype = EXT_SRCTYPE[ext]
            except KeyError:
                raise UserError('unknown file extension: {}'
                                .format(filepath))
            self.sources.append(TagSourceFile(filepath, srctags, sourcetype))
