"""Source files.

All source files must be contained in groups.  Groups do not nest.
All paths use the POSIX path convention (with a '/' separator), even
on Windows.  The platform-specific build code is expected to convert
the slashes to backslashes as necessary.
"""
import os
import gen.path
import re

_VALID_NAME = re.compile('^[A-Za-z](?:[-_A-Za-z0-9]*[A-Za-z0-9])$')

PLATFORMS = frozenset(['LINUX', 'MACOSX', 'WINDOWS'])

class Source(object):
    """A source file.

    Has a path, which is relative to the group.  Each source file also
    has a set of 'atoms', which are strings that describe how the
    source file is to be compiled.

    You should not construct a Source object directly, instead call
    the 'add' method on a Group object.
    """
    def __init__(self, path, atoms, group):
        if not isinstance(path, gen.path.Path):
            raise TypeError('path must be Path instance')
        if not isinstance(atoms, tuple):
            raise TypeError('atoms must be tuple')
        if not isinstance(group, Group):
            raise TypeError('group must be Group instance')
        self._path = path
        self._atoms = atoms
        self._group = group
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
    def match_platform(self, plat):
        """Return whether this file is built on the platform.

        If this file has no platform atoms, then this function returns
        true.  Otherwise, this function only returns true if the
        parameter matches one of the platform atoms.
        """
        plats = set(self._atoms) & PLATFORMS
        return (not plats) or (plat in plats)
    def other_atoms(self):
        """Iterate over non-platform atoms."""
        p = PLATFORMS
        for a in self._atoms:
            if a not in p:
                yield a

class Group(object):
    """A group of source files.

    Each group has a root path, a name, and a set of atoms.
    """
    def __init__(self, name, path, atoms):
        if not _VALID_NAME.match(name):
            raise ValueError('invalid group name: %r' % (name,))
        self._name = name
        self._path = path
        self._sources = []
        self._atoms = list(atoms)
        self._paths = set()
    def add(self, path, atoms):
        """Add a source file to the group."""
        if path in self._paths:
            raise ValueError('duplicate path in group %s: %s' %
                             (self._name, path.posixpath))
        self._paths.add(path)
        atoms = list(atoms)
        atoms.extend(self._atoms)
        src = Source(path, tuple(atoms), self)
        self._sources.append(src)
    def __repr__(self):
        return '<group name=%r root=%r>' % (self._name, self._path)
    @property
    def name(self):
        """The name of the group."""
        return self._name
    @property
    def relpath(self):
        """The path of the group relative to the top level."""
        return self._path
    def sources(self):
        """Iterate over all sources in the group."""
        return iter(self._sources)

class SourceList(object):
    """A list of all source files in a project.

    Logically consists of a list of source groups.
    """
    def __init__(self):
        self._groups = []
    def add_group(self, group):
        for g in self._groups:
            if g.name == group.name:
                raise Exception('group exists with name %r' % (group.name,))
        self._groups.append(group)
    def read_list(self, name, path, atoms):
        """Read a list of source files at the given path.

        If there is an existing group with the same name and same root
        directory, then the files are added to the group.  Otherwise,
        creates a new group for the source files using the given name.
        The path to the source file list should use the native OS
        conventions, but it must be a relative path.
        """
        dname, fname = os.path.split(path)
        gpath = gen.path.Path(dname.replace(os.path.sep, '/'))
        for g in self._groups:
            if g.relpath == gpath:
                if g.name == name:
                    break
            else:
                if g.name == name:
                    raise Exception(
                        'group %r has two paths: %r and %r' %
                        (name, gpath, g.relpath))
        else:
            g = Group(name, gpath, atoms)
            self._groups.append(g)
        for line in open(path, 'r'):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split()
            path, atoms = parts[0], parts[1:]
            path = gen.path.Path(path)
            g.add(path, atoms)
    def sources(self):
        """Iterate over all sources in the source list."""
        for g in self._groups:
            for s in g.sources():
                yield s
    def atoms(self):
        """Get a frozenset of all atoms used by any source."""
        atoms = set()
        for g in self._groups:
            for s in g.sources():
                atoms.update(s.atoms)
        return frozenset(atoms)
