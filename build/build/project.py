from build.path import Path, Href
import build.path
import collections
import os
import posixpath

class Project(object):
    __slots__ = ['srcdir', 'environ', 'modules', 'info', 'counter',
                 'files']

    def __init__(self, srcdir, environ):
        self.srcdir = srcdir
        self.environ = environ
        self.modules = []
        self.info = None
        self.counter = 0
        self.files = []

    def base_path(self, base):
        if base == 'srcdir':
            return self.srcdir
        elif base == 'builddir':
            return os.path.curdir
        else:
            assert ValueError('invalid base')

    def path(self, path, base='srcdir'):
        """Convert a native path to a path object."""
        rel_path = os.path.relpath(path, self.base_path(base))
        drive, dpath = os.path.splitdrive(rel_path)
        try:
            if drive or os.path.isabs(dpath):
                raise ValueError(
                    'path not contained in ${{{}}}' .format(base))
            return Path('/', base).join(dpath)
        except ValueError as ex:
            raise ValueError('{}: {!r}'.format(ex, path))

    def native(self, path):
        """Convert a path object to a native path."""
        ppath = path.path[1:] or os.path.curdir
        if path.base == 'srcdir':
            bpath = self.srcdir
        else:
            bpath = os.path.curdir
        if bpath == os.path.curdir:
            return ppath
        if ppath == os.path.curdir:
            return bpath
        return os.path.join(bpath, ppath)

    def posix(self, path, base='builddir'):
        """Convert a path object to a POSIX path relative to base."""
        pbase = path.base
        ppath = path.path[1:]
        if base == pbase:
            return ppath or '.'
        elif pbase == 'srcdir' and base == 'builddir':
            bpath = self.srcdir
            if bpath == '.':
                return ppath or '.'
            if ppath:
                return posixpath.join(bpath, ppath)
            return bpath
        else:
            raise ValueError('cannot convert path bases')

    def load_xml(self, path):
        """Load a project from the given path."""
        import build.xml as xml

        path = self.path(path)
        basename, ext = path.splitext()
        if ext != '.xml':
            raise ValueError('expected .xml extension for project file')
        first = True

        q = [basename]
        paths = set(q)
        refmap = {}
        while q:
            xmlpath = q.pop()
            self.files.append(xmlpath.addext('.xml'))
            buildfile = xml.parse_file(self, xmlpath)
            ref = Href(xmlpath, None)
            if buildfile.default is not None and buildfile.default != ref:
                refmap[ref] = buildfile.default
            buildfile.expand_templates(self)
            if xmlpath == basename:
                self.info = buildfile.info
            npaths = set()
            for m in buildfile.modules:
                npaths.update(ref.path for ref in m.module_refs())
                self.modules.append(m)
            npaths.discard(None)
            npaths.difference_update(paths)
            q.extend(npaths)
            paths.update(npaths)

        for m in self.modules:
            m.update_refs(refmap)

        modnames = set()
        for m in self.modules:
            if m.name is not None:
                if m.name in modnames:
                    raise ValueError('duplicate module name: {}'
                                     .format(m.name))
                modnames.add(m.name)

        for m in self.modules:
            for ref in m.module_refs():
                if ref not in modnames:
                    raise ValueError('undefined module ref: {}'
                                     .format(ref))

    def dump_xml(self, fp):
        """Dump a project as XML."""
        import build.xml as xml
        xml.dump(self.modules, self.info, fp)

    def gen_name(self):
        """Generate a unique module name."""
        self.counter += 1
        return Href(None, str(self.counter))
