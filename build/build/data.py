import collections
import urllib.parse

Source = collections.namedtuple('Source', 'path type')
Requirement = collections.namedtuple('Requirement', 'module public')
HeaderPath = collections.namedtuple('HeaderPath', 'path public')

class Module(object):
    __slots__ = ['name', 'type', 'group', 'info', 'modules']
    def __init__(self, name, type):
        self.name = name
        self.type = type
        self.group = Group()
        self.info = {}
        self.modules = []
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

class Group(object):
    __slots__ = ['sources', 'requirements', 'header_paths', 'groups']
    def __init__(self):
        self.sources = []
        self.requirements = []
        self.header_paths = []
        self.groups = []
    def all_groups(self):
        """Iterate over all groups and subgroups."""
        q = [self]
        while q:
            g = q.pop()
            yield g
            q.extend(g.groups)

class BuildFile(object):
    __slots__ = ['modules', 'info', 'default']
    def __init__(self):
        self.modules = []
        self.info = {}
        self.default = None
    def expand_templates(self, proj):
        self.modules = expand_templates(self.modules, self.info, proj)

def expand_templates(modules, info, proj):
    """Expand templates in the given modules."""
    expanded_modules = []
    q = list(modules)
    for module in q:
        module.modules = expand_templates(module.modules, info, proj)
    q.reverse()
    while q:
        module = q.pop()
        try:
            func = TEMPLATES[module.type]
        except KeyError:
            expanded_modules.append(module)
        else:
            expansion = list(func(module, proj))
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
