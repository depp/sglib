from ..shell import escape
import sys
import io

class Var(object):
    """A build variable."""
    __slots__ = ['name']

    def __init__(self, name, doc):
        self.name = name

    def __get__(self, instance, owner):
        if instance is None:
            return self
        try:
            return instance.variables[self.name]
        except KeyError:
            raise AttributeError(
                'Build variable not set: {}'.format(self.name))

    def __set__(self, instance, value):
        instance.variables[self.name] = value

    def __delete__(self, instance):
        del instance.variables[self.name]

class VarProg(Var):
    """A build variable which stores the name or path of a program."""

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

class VarFlags(Var):
    """A build variable which stores command-line flags.."""

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

class VarUniqueFlags(VarFlags):
    """A build variable which stores unique command-line flags."""

    @staticmethod
    def combine(x):
        s = set()
        a = []
        for i in x:
            for j in i:
                if j not in s:
                    s.add(j)
                    a.append(j)
        return tuple(a)

class VarPaths(Var):
    """A build variable which stores paths."""

    @staticmethod
    def combine(x):
        paths = set()
        a = []
        for i in x:
            for p in i:
                if p not in paths:
                    paths.add(p)
                    a.append(p)
        return tuple(a)

    @staticmethod
    def show(x):
        return ' '.join(i.posix for i in x)

class VarDefs(Var):
    """A build variable which stores preprocessor definitions."""

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

class BuildVariables(object):
    """A set of build variables."""
    __slots__ = ['variables']

    CC          = VarProg('CC', 'The C compiler')
    CXX         = VarProg('CXX', 'The C++ compiler')
    LD          = VarProg('LD', 'The linker')
    CPPFLAGS    = VarFlags('CPPFLAGS', 'C and C++ preprocessor flags')
    CPPPATH     = VarPaths('CPPPATH', 'C and C++ header search paths')
    DEFS        = VarDefs('DEFS', 'C and C++ preprocessor definitions')
    CFLAGS      = VarFlags('CFLAGS', 'C compilation flags')
    CXXFLAGS    = VarFlags('CXXFLAGS', 'C++ compilation flags')
    CWARN       = VarFlags('CWARN', 'C warning flags')
    CXXWARN     = VarFlags('CXXWARN', 'C++ warning flags')
    LDFLAGS     = VarFlags('LDFLAGS', 'Linker flags')
    LIBS        = VarFlags('LIBS', 'Linker flags specifying libraries')
    FPATH       = VarPaths('FPATH', 'Framework search paths')
    FRAMEWORKS  = VarUniqueFlags('FRAMEWORKS', 'Frameworks to link in')

    @classmethod
    def variable_definition(class_, name):
        """Get the definition for a variable."""
        try:
            vardef = getattr(class_, name)
        except AttributeError:
            return None
        if not isinstance(vardef, Var):
            return None
        return vardef

    def __init__(self, **kw):
        """Create a set of build variables."""
        for varname, value in kw.items():
            vardef = self.variable_definition(varname)
            if vardef is None:
                raise ValueError('unknown variable: {}'.format(varname))
        self.variables = kw

    @classmethod
    def parse(class_, vardict):
        """Create a set of build variables from strings."""
        variables = {}
        for varname, value in vardict.items():
            variables[varname] = class_.parse_variable(varname, value)
        return class_(**variables)

    @classmethod
    def parse_variable(class_, varname, value):
        """Parse a variable."""
        vardef = class_.variable_definition(varname)
        if vardef is None:
            raise ValueError('unknown variable: {}'.format(varname))
        try:
            parse = vardef.parse
        except AttributeError:
            raise ValueError('cannot parse variable: {}'.format(varname))
        return parse(value)

    @classmethod
    def merge(class_, varsets):
        """Merge a set of build variable sets."""
        variables = {}
        for varset in varsets:
            for varname, value in varset.variables.items():
                try:
                    values = variables[varname]
                except KeyError:
                    variables[varname] = [value]
                else:
                    values.append(value)
        obj = class_()
        for varname, values in variables.items():
            if len(values) == 1:
                value = values[0]
            else:
                value = getattr(class_, varname).combine(values)
            obj.variables[varname] = value
        return obj

    def dump(self, file=None, *, indent=''):
        """Dump the build variables to a file.

        This is for human consumption, not for serialization.
        """
        if file is None:
            file = sys.stdout
        for varname, value in sorted(self.variables.items()):
            vardef = getattr(self.__class__, varname)
            print('{}{}: {}'
                  .format(indent, varname, vardef.show(value)),
                  file=file)

    def __repr__(self):
        return 'BuildVariables({})'.format(
            ', '.join('{}={!r}'.format(varname, value)
                      for varname, value in self.variables.items()))

    def __getitem__(self, key):
        return self.variables[key]

    def __setitem__(self, key, value):
        if self.variable_definition(key) is None:
            raise KeyError(repr(key))
        self.variables[key] = value

    def __delitem__(self, key):
        del self.variables[key]

    def __iter__(self):
        return iter(self.variables)

    def __len__(self):
        return len(self.variables)

    def __contains__(self, key):
        return key in self.variables

    def keys(self):
        return self.variables.keys()

    def items(self):
        return self.variables.items()

    def values(self):
        return self.variables.values()

    def get(self, key, default=None):
        return self.variables.get(key, default)

    def __eq__(self, other):
        return (isinstance(other, BuildVariables) and
            self.variables == other.variables)

    def __ne__(self, other):
        return not self.__eq__(other)

    def update(self, **kw):
        """Update variables with the keyword arguments."""
        for varname, value2 in kw.items():
            vardef = self.variable_definition(varname)
            if vardef is None:
                raise ValueError('unknown variable: {}'.format(varname))
            try:
                value1 = self.variables[varname]
            except KeyError:
                self.variables[varname] = value2
            else:
                self.variables[varname] = vardef.combine([value1, value2])

    def update_parse(self, variables, *, strict):
        """Update variables from a dictionary, parsing the values."""
        for varname, value2 in variables.items():
            try:
                value2 = self.parse_variable(varname, value2)
            except ValueError:
                if strict:
                    raise
                continue
            try:
                value1 = self.variables[varname]
            except KeyError:
                self.variables[varname] = value2
            else:
                vardef = self.variable_definition(varname)
                self.variables[varname] = vardef.combine([value1, value2])

if __name__ == '__main__':
    x = BuildVariables.parse({
        'CC': 'gcc',
        'CFLAGS': '-O2 -g',
    })
    print(x)
