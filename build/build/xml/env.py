from build.path import Path
import build.data as data
from build.expr import evaluate, evaluate_many, to_bool, compare

# Tags and the attributes that can appear on them
TAGS = {
    k: frozenset(v.split()) for k, v in {
    'case': 'value',
    'if': 'value',
    'switch': 'value',

    'build': 'path',
    'group': 'path',
    'module': 'path name type',

    'header-path': 'path public',
    'key': 'name',
    'path': 'path',
    'require': 'module public',
    'src': 'path generator',
}.items()}

def get_path(base, attrib, required=False):
    """Get the path from an attribute dictionary.

    Make it relative to the base path.  If 'required' is False, then
    this function will return the base path if no path is in the
    attribute dictionary.
    """
    try:
        path = attrib['path']
    except KeyError:
        pass
    else:
        return Path(base, path)
    if required:
        raise ValueError('missing attribute: path')
    return base

def get_bool(attrib, key, default=False):
    """Get a boolean from an attribute dictionary."""
    try:
        val = attrib[key]
    except KeyError:
        return default
    if val == 'true':
        return True
    elif val == 'false':
        return False
    raise ValueError('invalid boolean: {!r}'.format(val))

def get_attrib(attrib, key):
    try:
        return attrib[key]
    except KeyError:
        pass
    raise ValueError('missing attribute: {}'.format(key))

class Env(object):
    """Environment in which XML tags are interpreted.

    Abstract base class.
    """
    __slots__ = []

    def add_str(self, val):
        if not val.isspace():
            raise ValueError('unexpected character data: {!r}'
                             .format(val.strip()))

    def add_node(self, tag, attrib, loc):
        try:
            allowed_attribs = TAGS[tag]
        except KeyError:
            raise ValueError('unknown tag: <{}>'.format(tag))
        try:
            func = getattr(self, 'add_' + tag.replace('-', '_'))
        except AttributeError:
            raise ValueError('unexpected tag: <{}>'.format(tag))
        extra_attribs = set(attrib).difference(allowed_attribs)
        if extra_attribs:
            raise ValueError('unexpected attributes: {}'
                             .format(', '.join(sorted(extra_attribs))))
        return func(attrib, loc)

    def finish(self):
        pass

# This will raise an error if you try to put anything inside
null_env = Env()

class ConditionalMixin(object):
    """Mixin for environments that can contain conditionals.

    Must have a 'vars' attribute.
    """
    __slots__ = []

    def add_if(self, attrib, loc):
        value = attrib.get('value')
        if value is None:
            raise ValueError('missing attribute: value')
        if to_bool(evaluate(self.vars, value)):
            return self

    def add_switch(self, attrib, loc):
        value = attrib.get('value')
        if value is not None:
            value = evaluate(self.vars, value)
        return SwitchEnv(self.vars, self, value)

class GroupMixin(object):
    """Mixin for group environments (group and module).

    Requires 'vars', 'path', and 'group' attributes.
    """
    __slots__ = []

    def add_group(self, attrib, loc):
        path = get_path(self.path, attrib)
        group = data.Group()
        self.group.groups.append(group)
        return GroupEnv(self.vars, path, group)

    def add_src(self, attrib, loc):
        path = get_path(self.path, attrib, required=True)
        generator = attrib.get('generator')
        self.group.sources.append(data.Source(path, generator))
        return null_env

    def add_header_path(self, attrib, loc):
        path = get_path(self.path, attrib, required=True)
        public = get_bool(attrib, 'public')
        self.group.header_paths.append(data.HeaderPath(path, public))
        return null_env

    def add_require(self, attrib, loc):
        module = get_attrib(attrib, 'module')
        public = get_bool(attrib, 'public')
        for m in module.split():
            self.group.requirements.append(data.Requirement(m, public))
        return null_env

class InfoMixin(object):
    """Mixin for environments with info keys (build and module).

    Requires 'vars', 'path', and 'info' attributes.
    """
    __slots__ = []

    def add_key(self, attrib, loc):
        name = get_attrib(attrib, 'name')
        return KeyEnv(self.vars, self.path, self.info, name)

class ModuleMixin(object):
    """Mixin for environments containing modules."""
    __slots__ = []
    def add_module(self, attrib, loc):
        path = get_path(self.path, attrib)
        name = attrib.get('name')
        type = attrib.get('type', 'module')
        module = data.Module(name, type)
        self.modules.append(module)
        return ModuleEnv(self.vars, path, module)

########################################

class SwitchEnv(Env):
    """Environment for switch tags."""
    __slots__ = ['vars', 'dest', 'value', 'handled']

    def __init__(self, vars, dest, value):
        self.vars = vars
        self.dest = dest
        self.value = value
        self.handled = False

    def add_case(self, attrib, loc):
        value = attrib.get('value')
        if self.handled:
            branch = False
        elif value is None:
            branch = True
        else:
            values = evaluate_many(self.vars, value)
            if self.value is None:
                branch = any(to_bool(v) for v in values)
            else:
                branch = any(compare(v, self.value) for v in values)
        if branch:
            self.handled = True
            return self.dest

    def finish(self):
        if not self.handled:
            if self.value:
                raise ValueError('unhandled switch: {!r}'.format(self.value))
            raise ValueError('unhandled switch')
        super(SwitchEnv, self).finish()

class RootEnv(Env, ModuleMixin):
    """Environment for the document root."""
    __slots__ = ['vars', 'path', 'buildfile']

    def __init__(self, vars, path, buildfile):
        self.vars = vars
        self.path = path
        self.buildfile = buildfile

    def add_build(self, attrib, loc):
        return BuildEnv(self.vars, get_path(self.path, attrib),
                        self.buildfile)

    @property
    def modules(self):
        return self.buildfile.modules

class BuildEnv(Env, ConditionalMixin, InfoMixin, ModuleMixin):
    """Environment for build tags."""
    __slots__ = ['vars', 'path', 'buildfile']

    def __init__(self, vars, path, buildfile):
        self.vars = vars
        self.path = path
        self.buildfile = buildfile

    @property
    def info(self):
        return self.buildfile.info

    @property
    def modules(self):
        return self.buildfile.modules

class ModuleEnv(Env, ConditionalMixin, GroupMixin, InfoMixin):
    """Environment for module tags."""
    __slots__ = ['vars', 'path', 'module']

    def __init__(self, vars, path, module):
        self.vars = vars
        self.path = path
        self.module = module

    @property
    def group(self):
        return self.module.group

    @property
    def info(self):
        return self.module.info

class GroupEnv(Env, ConditionalMixin, GroupMixin):
    """Environment for group tags."""
    __slots__ = ['vars', 'path', 'group']

    def __init__(self, vars, path, group):
        self.vars = vars
        self.path = path
        self.group = group

class KeyEnv(Env, ConditionalMixin):
    """Environment for key tags."""
    __slots__ = ['vars', 'path', 'info', 'name', 'val']

    def __init__(self, vars, path, info, name):
        self.vars = vars
        self.path = path
        self.info = info
        self.name = name
        self.val = []

    def add_str(self, val):
        self.val.append(val)

    def add_path(self, attrib, loc):
        path = get_path(self.path, attrib, required=True)
        self.val.append(path)
        return null_env
