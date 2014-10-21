# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
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
        return ' '.join(i for i in x)

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
    __slots__ = ['schema', 'variables']

    def __init__(self, schema, **kw):
        """Create a set of build variables."""
        for varname, value in kw.items():
            if varname not in schema:
                raise ValueError('unknown variable: {}'.format(varname))
        self.schema = schema
        self.variables = kw

    @classmethod
    def parse(class_, schema, vardict, *, strict):
        """Create a set of build variables from strings."""
        variables = {}
        for varname, value in vardict.items():
            try:
                value = class_.parse_variable(schema, varname, value)
            except ValueError:
                if strict:
                    raise
            else:
                variables[varname] = value
        return class_(schema, **variables)

    @staticmethod
    def parse_variable(schema, varname, value):
        """Parse a variable."""
        try:
            vardef = schema[varname]
        except KeyError:
            raise ValueError('unknown variable: {}'.format(varname))
        try:
            parse = vardef.parse
        except AttributeError:
            raise ValueError('cannot parse variable: {}'.format(varname))
        return parse(value)

    @classmethod
    def merge(class_, schema, varsets):
        """Merge a set of build variable sets."""
        lists = {}
        for varset in varsets:
            if varset is None:
                continue
            if varset.schema is not schema:
                raise ValueError('schema mismatch')
            for varname, value in varset.variables.items():
                try:
                    values = lists[varname]
                except KeyError:
                    lists[varname] = [value]
                else:
                    values.append(value)
        variables = {}
        for varname, value in variables.items():
            if len(value) == 1:
                value = value[0]
            else:
                value = schema[varname].combine(values)
            variables[varname] = value
        return class_(schema, **variables)

    def dump(self, file=None, *, indent=''):
        """Dump the build variables to a file.

        This is for human consumption, not for serialization.
        """
        if file is None:
            file = sys.stdout
        for varname, value in sorted(self.variables.items()):
            vardef = self.schema[varname]
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
        if key not in self.schema:
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
            try:
                vardef = self.schema[varname]
            except KeyError:
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
                vardef = self.schema[varname]
                self.variables[varname] = vardef.combine([value1, value2])

if __name__ == '__main__':
    x = BuildVariables.parse({
        'CC': 'gcc',
        'CFLAGS': '-O2 -g',
    })
    print(x)
