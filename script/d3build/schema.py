# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .shell import escape
from .error import ConfigError, UserError
import sys

class Schema(object):
    """A schema is a description of variables affecting a build system.

    This is an abstract base class.
    """
    __slots__ = [
        # Map from variable names to definitions.
        '_variables',
        # Default variable definition.
        '_default',
        # List of target architectures.
        'archs',
        # List of build configurations.
        'configs',
        # List of build variants.
        'variants',
        # Map from flag names to variable names.
        'flags',
    ]

    def __init__(self, *, variables, default=None,
                 archs, configs, variants, flags):
        self._variables = variables
        self._default = default
        self.archs = archs
        self.configs = configs
        self.variants = variants
        self.flags = flags

    def get_variable(self, name):
        """Get the definition for a variable.

        Raises ValueError if no variable with that name exists.
        """
        try:
            result = self._variables[name]
        except KeyError:
            pass
        else:
            return result[0]
        default = self._default
        if default is None:
            raise ValueError('unknown variable: {!r}'.format(name))
        return default

    def get_variants(self, configs=None, archs=None):
        """Get a list of variants."""
        raise NotImplementedError('must be implemented by subclass')

    def parse_name(self, name):
        """Parse the name of a variable.

        Returns a sequence of (variant, name) tuples, or returns None if
        the name is not a valid name.
        """
        return None

class SchemaBuilder(object):
    """Object used for creating new schemas.

    The 'sep' parameter determines the separator for list variables,
    which is ';' for Visual Studio and None for Xcode and Gnu Make.  A
    value of None indicates that lists are separated with whitespace.

    The 'bool_values' parameter determines how boolean values are
    printed.
    """
    __slots__ = ['_variables', 'sep', 'bool_values']

    def __init__(self, sep=None, bool_values=('False', 'True')):
        self._variables = {}
        self.sep = sep
        self.bool_values = bool_values

    def value(self):
        """Get the variable definitions."""
        return self._variables

def _vartype(name):
    def func(class_):
        def add_var(self, name, doc=None, **kw):
            self._variables[name] = (class_(self, **kw), doc)
            return self
        setattr(SchemaBuilder, name, add_var)
        return class_
    return func

@_vartype('string')
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

@_vartype('bool')
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

@_vartype('list')
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

@_vartype('defs')
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

class Variables(object):
    """A set of build variables.

    The variables may have different values for different build variants.
    """
    __slots__ = ['_schema', '_varsets']

    def __init__(self, schema, varsets):
        self._schema = schema
        self._varsets = varsets

    def get(self, variant, name, default=None):
        """Get the value of a variable.

        If the default value is None, then an exception will be thrown if
        the variable is not set.
        """
        if variant not in self._schema.variants:
            raise ValueError('invalid variant: {!r}'.format(variant))
        vardef = self._schema.get_variable(name)
        key = variant, name
        values = []
        for varset in self._varsets:
            try:
                value = varset[key]
            except KeyError:
                continue
            values.append(value)
        if not values:
            if default is None:
                raise ValueError('variable is not set: {!r} (variant={})'
                                 .format(name, variant))
            return default
        if len(values) == 1:
            return values[0]
        return vardef.combine(values)

    def get_all(self):
        """Get a map from variants to the variables for that variant."""
        lists = {variant: {} for variant in self._schema.variants}
        for varset in self._varsets:
            for var, value in varset.items():
                var_variant, name = var
                vlists = lists[var_variant]
                try:
                    values = vlists[name]
                except KeyError:
                    vlists[name] = [value]
                else:
                    values.append(value)
        result = {}
        for variant, vlists in lists.items():
            vresult = {}
            result[variant] = vresult
            for name, values in vlists.items():
                if len(values) == 1:
                    value = values[0]
                else:
                    vardef = self._schema.get_variable(name)
                    value = vardef.combine(values)
                vresult[name] = value
        return result

    def dump(self, *, file=None, indent=''):
        """Dump a set of variables.

        This is for human consumption, not for serialization.
        """
        if file is None:
            file = sys.stdout
        for variant, vvars in self.get_all().items():
            print('{}Variant {}:'.format(indent, variant), file=file)
            for name, value in vvars.items():
                vardef = self._schema.get_variable(name)
                text = vardef.show(value)
                print('{}  {}: {}'.format(indent, name, text), file=file)
