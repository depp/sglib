# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..shell import escape

class Schema(object):
    """A schema is a description of variables affecting a build system."""
    __slots__ = ['variables', 'lax']

    def __init__(self, *, lax=False):
        self.variables = {}
        self.lax = lax

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

    def varset(self, **kw):
        self.validate(kw)
        return kw

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
        for varname, value in variables.items():
            if len(value) == 1:
                value = value[0]
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

    def update(self, varset1, **kw):
        """Modify a variable set by merging new variables."""
        for varname, value2 in kw.items():
            try:
                vardef = self[varname]
            except KeyError:
                raise ValueError('unknown variable: {!r}'.format(varname))
            if not vardef.isvalid(value2):
                raise ValueError('invalid value: key={}, value={!r}'
                                 .format(varname, value2))
            try:
                value1 = varset1[varname]
            except KeyError:
                varset1[varname] = value2
            else:
                varset1[varname] = vardef.combine([value1, value2])

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
            self.variables[name] = (class_(**kw), doc)
            return self
        setattr(Schema, name, add_var)
        return class_
    return func

@_schema('string')
class VarString(object):
    """A build variable which stores a string value."""
    __slots__ = []

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

class BaseList(object):
    __slots__ = []

    @staticmethod
    def parse(string):
        return string.split()

    @staticmethod
    def show(value):
        return ' '.join(escape(item) for item in value)

    @staticmethod
    def isvalid(value):
        return (isinstance(value, list) and
                all(isinstance(item, str) for item in value))

@_schema('list')
class VarList(BaseList):
    """A build variable which stores a list."""
    __slots__ = ['_unique']

    def __init__(self, *, unique=False):
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
