from gen.shell import escape
import gen.project as project

class MakefileVar(object):
    """A makefile variable."""

    __slots__ = ['name']

    def __init__(self, name):
        self.name = name

class VarBool(object):
    """An environment variable for a boolean."""

    @staticmethod
    def combine(x):
        v = False
        for i in x:
            if i is not None:
                v = i
        return v

    @staticmethod
    def show(x):
        if x:
            return 'Yes'
        return ''

class VarProg(object):
    """An environment variable for a program path."""

    @staticmethod
    def parse(s):
        return s

    @staticmethod
    def combine(x):
        v = None
        for i in x:
            if i is not None:
                v = i
        return v

    @staticmethod
    def show(x):
        if x is None:
            return ''
        return x

class VarFlags(object):
    """An environment variable for program flags."""

    @staticmethod
    def parse(s):
        return tuple(s.split())

    @staticmethod
    def combine(x):
        a = []
        for i in x:
            a.extend(i)
        return tuple(a)

    @staticmethod
    def show(x):
        return ' '.join(escape(i) for i in x)

class VarPathList(object):
    """An environment variable for path lists."""

    @staticmethod
    def combine(x):
        paths = set()
        a = []
        for i in x:
            for p in i:
                if p.posix not in paths:
                    paths.add(p.posix)
                    a.append(p)
        return tuple(a)

    @staticmethod
    def show(x):
        return ' '.join(i.posix for i in x)

class VarDefs(object):
    """An environment variable for preprocessor definitions."""

    @staticmethod
    def combine(x):
        names = set()
        a = []
        for i in x:
            for k, v in i:
                if k not in names:
                    names.add(k)
                    a.append((k, v))
        return tuple(a)

    @staticmethod
    def show(x):
        return ' '.join('{}={}'.format(k, v) for k, v in x)

class VarCondition(object):
    """An environment variable specifying when something is built."""

    @staticmethod
    def combine(x):
        names = set()
        for i in x:
            names.update(i)
        return tuple(sorted(names))

    @staticmethod
    def show(x):
        return ' '.join(x)

VAR = {
    # Build tools
    'CC':   VarProg,
    'CXX':  VarProg,
    'LD':   VarProg,

    # Compliation flags
    'CPPFLAGS': VarFlags,
    'CPPPATH':  VarPathList,
    'DEFS':     VarDefs,
    'CFLAGS':   VarFlags,
    'CXXFLAGS': VarFlags,
    'CWARN':    VarFlags,
    'CXXWARN':  VarFlags,
    'LDFLAGS':  VarFlags,
    'LIBS':     VarFlags,

    # condition: list of tags which are necessary in this environment
    'condition': VarCondition,

    # external: if true, disable warnings
    'external': VarBool,
}

def parse_env(d):
    e = {}
    for varname, vardef in VAR.items():
        try:
            p = vardef.parse
        except AttributeError:
            continue
        try:
            x = d[varname]
        except KeyError:
            continue
        e[varname] = p(x)
    return e

def merge_env(a):
    if not a:
        return {}
    if len(a) == 1:
        return a[0]
    e = {}
    for varname, vardef in VAR.items():
        x = []
        for d in a:
            try:
                x.append(d[varname])
            except KeyError:
                pass
        if x:
            if len(x) > 1:
                x = vardef.combine(x)
            else:
                x = x[0]
            e[varname] = x
    return e

class MergeEnvironment(object):
    """An environment dictionary made from combining other dictionaries.

    Each key is merged according to its type.
    """

    # _env: list of environment dictionaries
    __slots__ = ['_env']

    def __init__(self, env):
        self._env = env

    def __getitem__(self, key):
        try:
            v = VAR[key]
        except KeyError:
            raise KeyError(key)
        a = []
        for e in self._env:
            try:
                x = e[key]
            except KeyError:
                pass
            else:
                a.append(x)
        return v.combine(a)

    def dump(self):
        for k, v in VAR.items():
            print('{} = {}'.format(k, self[k]))

class BuildEnv(object):
    """A BuildEnv maps tags to build environments.

    Takes a callback function as a parameter.  The function takes a
    library source as a parameter and returns an envirenment.  The
    function can be arbitrarily expensive, the results will
    automatically be cached.
    """

    __slots__ = ['project', 'base_env', 'func', '_cache']

    def __init__(self, project, base_env, func):
        self.project = project
        self.base_env = base_env
        self.func = func
        self._cache = {'.external': {'external': True}}

    def _env1(self, tag):
        m = self.project.module_names[tag]
        if isinstance(m, project.ExternalLibrary) and not m.use_bundled:
            srcs = None
            for s in m.libsources:
                if isinstance(s, project.LibraryGroup):
                    srcs = s.libsources
                else:
                    srcs = [s]
                break
            # FIXME error
            assert srcs is not None
            a = []
            for s in srcs:
                a.append(self.func(s))
            e = merge_env(a)
        else:
            e = {}
            if m.header_path:
                e['CPPPATH'] = tuple(m.header_path)
            if m.define:
                e['DEFS'] = tuple(m.define)
        return e

    def env(self, tags):
        """Get the environment for a given set of tags.

        Returns an Env object, or None if one or more tags is disabled
        in this configuration.
        """
        a = [self.base_env]
        for tag in tags:
            try:
                e = self._cache[tag]
            except KeyError:
                e = self._env1(tag)
                self._cache[tag] = e
            assert e is not None
            a.append(e)
        return MergeEnvironment(a)
