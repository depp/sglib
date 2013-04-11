"""Project data.

These structures correspond to the project description on disk.  They
can be read from XML files, and they can be dumped to XML files
(although conditionals are lost).  The project data can also be
converted into build objects, which can be emitted using one of the
build system backends.
"""
import collections
import urllib.parse
from build.path import Path
from build.error import ConfigError

EXT_SRCTYPE = {'.' + ext: type for type, exts in {
    'c': 'c',
    'c++': 'cp cpp cxx',
    'header': 'h hpp hxx',
    'objc': 'm',
    'objc++': 'mm',
}.items() for ext in exts.split()}

Source = collections.namedtuple('Source', 'path type')
Requirement = collections.namedtuple('Requirement', 'module public')
HeaderPath = collections.namedtuple('HeaderPath', 'path public')

dummy = object()
def info_getter(func):
    def wrapped_getter(self, index, default=dummy):
        def fail(msg):
            nonlocal loc
            if loc is None:
                loc = self.loc
            raise ConfigError(str(msg), loc=loc)
        loc = None
        if not isinstance(index, str):
            raise TypeError('index must be string')
        try:
            (loc, val) = self._items[index]
        except KeyError:
            if default is dummy:
                fail('required key is missing: {}'.format(index))
            return default
        try:
            return func(val)
        except ValueError as ex:
            fail('{}: key={}'.format(ex, index))
    return wrapped_getter

class InfoDict(collections.MutableMapping):
    """An info dictionary.

    This keeps track of the locations of the keys in the source files,
    and does simple type conversions.
    """
    __slots__ = ['_items', 'loc']
    def __init__(self, loc=None):
        self._items = {}
        self.loc = loc
    def __len__(self):
        return len(self._items)
    def __contains__(self, index):
        return index in self._items
    def __iter__(self):
        return iter(self._items)
    def __getitem__(self, index):
        if not isinstance(index, str):
            raise TypeError('index must be string')
        return self._items[index][1]
    def __setitem__(self, index, val):
        self.set_key(index, val, None)
    def __delitem__(self, index):
        if not isinstance(index, str):
            raise TypeError('index must be string')
        del self._items[index]
    def __bool__(self):
        return bool(self._items)
    @info_getter
    def get_string(items):
        for item in items:
            if not isinstance(item, str):
                raise ValueError('expected string')
        return ''.join(items)
    @info_getter
    def get_path(items):
        """Convert an info dict item into a path."""
        if len(items) != 1 or not isinstance(items[0], Path):
            raise ValueError('expected single path')
        return items[0]
    def set_key(self, index, val, loc):
        if not isinstance(index, str):
            raise TypeError('index must be string')
        if not isinstance(val, (list, tuple)):
            raise TypeError('key value must be list or tuple')
        val = tuple(val)
        for item in val:
            if not isinstance(item, (str, Path)):
                raise TypeError('key value not a string or path: {}'
                                .format(type(val).__name__))
        self._items[index] = (loc, val)
    def prefix_path(self, path):
        """Return a copy with the paths prefixed."""
        obj = InfoDict(self.loc)
        for k, (loc, items) in self._items.items():
            nitems = []
            for item in items:
                if isinstance(item, Path):
                    item = item.prefix(path)
                nitems.append(item)
            nitems = tuple(nitems)
            obj._items[k] = (loc, nitems)
        return obj

class Module(object):
    __slots__ = ['name', 'type', 'group', 'info', 'modules', 'loc']
    def __init__(self, name, type, loc=None):
        self.name = name
        self.type = type
        self.group = Group(loc)
        self.info = InfoDict(loc)
        self.modules = []
        self.loc = loc
    def module_refs(self):
        """Iterate over all module references, including submodules."""
        for g in self.all_groups():
            for req in g.requirements:
                yield req.module
    def all_modules(self):
        """Iterate over all modules and submodules."""
        q = [self]
        while q:
            m = q.pop()
            yield m
            q.extend(m.modules)
    def all_groups(self):
        """Iterate over all groups and subgroups."""
        for m in self.all_modules():
            for g in m.group.all_groups():
                yield g
    def update_refs(self, refmap):
        """Update all hrefs using the given dictionary."""
        for g in self.all_groups():
            nreqs = []
            for req in g.requirements:
                try:
                    nref = refmap[req.module]
                except KeyError:
                    pass
                else:
                    req = Requirement(nref, req.public)
                nreqs.append(req)
            g.requirements = nreqs
    def prefix_path(self, path):
        """Return a copy with the paths prefixed."""
        obj = Module(self.name, self.type, self.loc)
        obj.group = self.group.prefix_path(path)
        obj.info = self.info.prefix_path(path)
        obj.modules = [mod.prefix_path(path) for mod in self.modules]
        return obj

class Group(object):
    __slots__ = ['sources', 'requirements', 'header_paths', 'groups', 'loc']
    def __init__(self, loc=None):
        self.sources = []
        self.requirements = []
        self.header_paths = []
        self.groups = []
        self.loc = loc
    def all_groups(self):
        """Iterate over all groups and subgroups."""
        q = [self]
        while q:
            g = q.pop()
            yield g
            q.extend(g.groups)
    def __bool__(self):
        return bool(self.sources or self.requirements or
                    self.header_paths or self.groups)
    def prefix_path(self, path):
        """Return a copy with the paths prefixed."""
        obj = Group(self.loc)
        obj.sources = [
            Source(src.path.prefix(path), src.type)
            for src in self.sources]
        obj.requirements = list(self.requirements)
        obj.header_paths = [
            HeaderPath(ipath.path.prefix(path), ipath.public)
            for ipath in self.header_paths]
        obj.groups = [group.prefix_path(path) for group in self.groups]
        return obj

class BuildFile(object):
    __slots__ = ['modules', 'info', 'default']
    def __init__(self):
        self.modules = []
        self.info = InfoDict()
        self.default = None
    def expand_templates(self, cfg, proj):
        self.modules = expand_templates(self.modules, self.info, cfg, proj)

def expand_templates(modules, info, cfg, proj):
    """Expand templates in the given modules."""
    expanded_modules = []
    q = list(modules)
    for module in q:
        module.modules = expand_templates(module.modules, info, cfg, proj)
    q.reverse()
    while q:
        module = q.pop()
        try:
            func = TEMPLATES[module.type]
        except KeyError:
            expanded_modules.append(module)
        else:
            expansion = list(func(module, info, cfg, proj))
            q.extend(reversed(expansion))
    return expanded_modules

TEMPLATES = {}
def template(*names):
    """Decorator for defining template module types."""
    def func(x):
        for name in names:
            TEMPLATES[name] = x
        return x
    return func
