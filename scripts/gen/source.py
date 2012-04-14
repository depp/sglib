"""Source files.

All source files must be contained in groups.  Groups do not nest.
"""
from __future__ import with_statement
import os
import gen.path
import re

_VALID_NAME = re.compile('^[A-Za-z0-9](?:[ -_A-Za-z0-9]*[A-Za-z0-9])$')
def valid_name(x):
    return _VALID_NAME.match(x) and '  ' not in x

_NONSIMPLE_CHAR = re.compile('[^-A-Za-z0-9]+')
def simple_name(x):
    return _NONSIMPLE_CHAR.sub('_', x).strip('-').lower()

_IS_ATOM = re.compile('^[_A-Za-z][_A-Za-z0-9]*$')
def is_atom(x):
    return bool(_IS_ATOM.match(x))

class Source(object):
    """A source file.

    Has a path, which is relative to the group.  Each source file also
    has a set of 'atoms', which are strings that describe how the
    source file is to be compiled.

    You should not construct a Source object directly, instead call
    the 'add' method on a Group object.
    """
    __slots__ = ['_path', '_atoms', '_group', '_sourcetype']

    def __init__(self, path, atoms, group):
        if not isinstance(path, gen.path.Path):
            raise TypeError('path must be Path instance')
        if not isinstance(atoms, tuple):
            raise TypeError('atoms must be tuple')
        if not isinstance(group, Group):
            raise TypeError('group must be Group instance')
        ext = path.ext
        if not ext:
            raise ValueError(
                'source file has no extension, cannot determine type: %s' %
                (path.posix,))
        try:
            stype = gen.path.EXTS[ext]
        except KeyError:
            raise ValueError(
                'source file %s has unknown extension, cannot determine type' %
                (path.posix,))
        self._path = path
        self._atoms = atoms
        self._group = group
        self._sourcetype = stype

    def __repr__(self):
        if self._atoms:
            return '<source %s %r atoms=%s>' % \
                (self._group.name, self._path.posix,
                 ', '.join(self._atoms))
        else:
            return '<source %s %r>' % \
                (self._group.name, self._path.posix)

    @property
    def group(self):
        """The group which this file belongs to."""
        return self._group

    @property
    def grouppath(self):
        """The path of the file, relative to the group."""
        return self._path

    @property
    def relpath(self):
        """The path of the file, relative to the top level."""
        return self._group.relpath / self._path

    @property
    def atoms(self):
        """The atoms which apply to this file, a tuple."""
        return self._atoms

    @property
    def sourcetype(self):
        """The type of this source file."""
        return self._sourcetype

class Group(object):
    """A group of source files.

    Each group has a root path, a name, and a list of sources.
    The name has two variants, the full version (which can have
    spaces) and the simple version (which can't).
    """
    __slots__ = ['_name', '_sname', '_path', '_sources', '_paths']

    def __init__(self, name, path):
        sname = simple_name(name)
        if not valid_name(name) or not sname:
            raise ValueError('invalid group name: %r' % (name,))
        if not isinstance(path, gen.path.Path):
            raise TypeError('group path must be Path')
        self._name = name
        self._sname = sname
        self._path = path
        self._sources = []
        self._paths = set()

    def add(self, path, atoms):
        """Add a source file to the group."""
        if path in self._paths:
            raise ValueError('duplicate path in group %s: %s' %
                             (self._name, path.posixpath))
        self._paths.add(path)
        src = Source(path, tuple(atoms), self)
        self._sources.append(src)
        return src

    def __repr__(self):
        return '<group name=%r root=%r>' % (self._name, self._path)

    @property
    def name(self):
        """The name of the group."""
        return self._name

    @property
    def simple_name(self):
        """The simple name of the group."""
        return self._sname

    @property
    def relpath(self):
        """The path of the group relative to the top level."""
        return self._path

    def sources(self):
        """Iterate over all sources in the group."""
        return iter(self._sources)

    def read_list(self, path, atoms):
        """Read a list of source files at the given path.
        
        All files are added to this group.  Paths are interpreted as relative
        to the group.
        """
        with open(path, 'r') as f:
            self._add_list(f, atoms)

    def read_list_str(self, text, atoms):
        """Add a list of source files from a given chunk of text."""
        self._add_list(text.splitlines(), atoms)

    def _add_list(self, lines, atoms):
        for line in lines:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split()
            spath, satoms = parts[0], parts[1:]
            spath = gen.path.Path(spath)
            self.add(spath, tuple(satoms) + atoms)

class SourceList(object):
    """A list of all source files in a project.

    Logically consists of a list of source groups.
    """
    __slots__ = ['_groups', '_gnames']
    def __init__(self):
        self._groups = []
        self._gnames = {}

    def groups(self):
        return self._gnames.itervalues()

    def get_group(self, name, path):
        """Get the group with the given name, creating it if necessary."""
        g = Group(name, path)
        sn = g.simple_name
        try:
            g = self._gnames[sn]
        except KeyError:
            self._groups.append(g)
            self._gnames[sn] = g
            return g
        if g.relpath != path:
            raise Exception(
                'group %r has two paths: %r and %r' %
                (name, path.posix, g.relpath.posix))
        if g.name != name:
            raise Exception(
                'name conflict between group %s and %s' %
                (name, g.name))
        return g

    def read_list(self, name, path, atoms):
        """Read a list of source files at the given path.

        Uses 'name' for the group name.  The group root path will be
        the folder containing the given file.  Merges data with
        existing groups.  Raises an exception if the group name
        conflicts with an existing group name that has a different
        path.
        """
        dname, fname = os.path.split(path)
        gpath = gen.path.Path(dname.replace(os.path.sep, '/'))
        g = self.get_group(name, gpath)
        g.read_list(path, atoms)

    def read_list_str(self, name, gpath, text, atoms):
        """Like read_list, but with a string instead of filename.

        The specified path is the root path for the group.
        """
        g = self.get_group(name, gpath)
        g.read_list_str(text, atoms)

    def sources(self):
        """Iterate over all sources in the source list."""
        for g in self._groups:
            for s in g.sources():
                yield s
