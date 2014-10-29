# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .shell import escape
from .error import ConfigError, UserError

class Schema(object):
    """A schema is a description of variables affecting a build system."""
    __slots__ = ['variables', 'lax', 'sep', 'bool_values']

    def __init__(self, *, lax=False, sep=None,
                 bool_values=('False', 'True')):
        self.variables = {}
        self.lax = lax
        self.sep = sep
        self.bool_values = bool_values

    def __getitem__(self, key):
        try:
            return self.variables[key][0]
        except KeyError:
            if self.lax:
                return VarString
            raise

    def __contains__(self, key):
        if self.lax:
            return True
        return key in self.variables

    def update_schema(self, other):
        """Update this schema by merging another schema."""
        if not isinstance(other, Schema):
            raise TypeError('object must be a schema')
        self.variables.update(other.variables)
        self.lax = self.lax or other.lax
        self.bool_values = other.bool_values

    def parse(self, vardict, *, strict):
        """Parse variables from strings."""
        variables = {}
        for varname, value in vardict.items():
            try:
                vardef = self[varname]
            except ValueError:
                if strict:
                    raise
                continue
            variables[varname] = vardef.parse(value)
        return variables

    def merge(self, varsets):
        """Merge sets of variables."""
        if not varsets:
            return {}
        if len(varsets) == 1:
            return varsets[0]
        lists = {}
        for varset in varsets:
            if not varset:
                continue
            for varname, value in varset.items():
                try:
                    values = lists[varname]
                except KeyError:
                    lists[varname] = [value]
                else:
                    values.append(value)
        variables = {}
        for varname, values in lists.items():
            if len(values) == 1:
                value = values[0]
            else:
                value = self[varname].combine(values)
            variables[varname] = value
        return variables

    def dump(self, varset, *, file=None, indent=''):
        """Dump a set of variables.

        This is for human consumption, not for serialization.
        """
        if file is None:
            file = sys.stdout
        for varname, value in sorted(varset.items()):
            vardef = self[varname]
            print('{}{}: {}'
                  .format(indent, varname, vardef.show(value)),
                  file=file)

    def _update1(self, varset, additions):
        if not additions:
            return
        if not isinstance(additions, dict):
            raise TypeError('additional variables must be a dictionary')
        for varname, value2 in additions.items():
            try:
                vardef = self[varname]
            except KeyError:
                raise ValueError('unknown variable: {!r}'.format(varname))
            if not vardef.isvalid(value2):
                raise ValueError('invalid value: key={}, value={!r}'
                                 .format(varname, value2))
            try:
                value1 = varset[varname]
            except KeyError:
                varset[varname] = value2
            else:
                varset[varname] = vardef.combine([value1, value2])

    def update(self, varset, *args, **kw):
        """Modify a variable set by merging new variables."""
        for arg in args:
            self._update1(varset, arg)
        self._update1(varset, kw)

    def validate(self, varset):
        """Validate the types of a set of variables."""
        for varname, value in varset.items():
            try:
                vardef = self[varname]
            except KeyError:
                raise ValueError('unknown variable: {!r}'.format(varname))
            if not vardef.isvalid(value):
                raise ValueError('invalid value: key={}, value={!r}'
                                 .format(varname, value))

def _schema(name):
    def func(class_):
        def add_var(self, name, doc=None, **kw):
            self.variables[name] = (class_(self, **kw), doc)
            return self
        setattr(Schema, name, add_var)
        return class_
    return func

@_schema('string')
class VarString(object):
    """A build variable which stores a string value."""
    __slots__ = []

    def __init__(self, schema):
        pass

    @staticmethod
    def parse(s):
        return s

    @staticmethod
    def combine(values):
        return values[-1]

    @staticmethod
    def show(value):
        return escape(value)

    @staticmethod
    def isvalid(value):
        return isinstance(value, str)

@_schema('bool')
class VarBool(object):
    """A build variable which stores a boolean value."""
    __slots__ = ['values']

    def __init__(self, schema):
        self.values = schema.bool_values

    @staticmethod
    def parse(s):
        v = s.lower()
        if v in ('false', 'no', 'off', '0'):
            return False
        if v in ('true', 'yes', 'on', '1'):
            return True
        raise ConfigError('invalid boolean: {!r}'.format(s))

    @staticmethod
    def combine(values):
        return values[-1]

    def show(self, value):
        return self.values[bool(value)]

    @staticmethod
    def isvalid(value):
        return isinstance(value, bool)

class BaseList(object):
    __slots__ = ['sep']

    def __init__(self, schema):
        self.sep = schema.sep

    def parse(self, string):
        return string.split(sep=self.sep)

    def show(self, value):
        if self.sep is None:
            return ' '.join(escape(item) for item in value)
        return self.sep.join(value)

    @staticmethod
    def isvalid(value):
        return (isinstance(value, list) and
                all(isinstance(item, str) for item in value))

@_schema('list')
class VarList(BaseList):
    """A build variable which stores a list."""
    __slots__ = ['_unique']

    def __init__(self, schema, *, unique=False):
        super(VarList, self).__init__(schema)
        self._unique = unique

    def combine(self, values):
        result = []
        if self._unique:
            itemset = set()
            for value in values:
                for item in value:
                    if item in itemset:
                        continue
                    itemset.add(item)
                    result.append(item)
        else:
            for value in values:
                result.extend(value)
        return result

@_schema('defs')
class VarDefs(VarList):
    """A build variable which stores preprocessor definitions."""
    __slots__ = []

    @staticmethod
    def combine(values):
        names = set()
        result = []
        for value in reversed(values):
            for item in reversed(value):
                i = item.find('=')
                name = item[:i] if i >= 0 else item
                if name in names:
                    continue
                names.add(name)
                result.append(item)
        result.reverse()
        return result

@_schema('objectlist')
class VarObjectList(object):
    """A build variable which stores a list of Python objects."""
    __slots__ = []

    def __init__(self, schema):
        pass

    @staticmethod
    def parse(s):
        raise UserError('this variable cannot be parsed')

    @staticmethod
    def combine(values):
        result = []
        for value in values:
            result.extend(value)
        return result

    def show(self, value):
        raise UserError('this variable cannot be shown')

    @staticmethod
    def isvalid(value):
        return isinstance(value, list)
