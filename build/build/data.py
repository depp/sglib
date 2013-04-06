import collections

Href = collections.namedtuple('Href', 'path frag')
Source = collections.namedtuple('Source', 'path type')
Requirement = collections.namedtuple('Requirement', 'module public')
HeaderPath = collections.namedtuple('HeaderPath', 'path public')

class Module(object):
    __slots__ = ['name', 'type', 'group', 'info']
    def __init__(self, name, type):
        self.name = name
        self.type = type
        self.group = Group()
        self.info = {}

class Group(object):
    __slots__ = ['sources', 'requirements', 'header_paths', 'groups']
    def __init__(self):
        self.sources = []
        self.requirements = []
        self.header_paths = []
        self.groups = []

class BuildFile(object):
    __slots__ = ['modules', 'info']
    def __init__(self):
        self.modules = []
        self.info = {}
