# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import collections
from build.error import ConfigError

Param = collections.namedtuple('Param', 'type default')

def parse_bool(x):
    y = x.lower()
    if y in ('yes', 'true', 'on', '1'):
        return True
    if y in ('no', 'false', 'off', '0'):
        return False
    raise ValueError('invalid boolean')

TYPES = {
    'bool': parse_bool,
}

class ParamParser(object):
    """Parser for parameter strings.

    Parameter strings appear in command line arguments, and have the form
    'key=val:key2=val2'.
    """
    __init__ = ['_params']
    def __init__(self):
        self._params = {}
    def add_param(self, name, *, type=None, default=None):
        if isinstance(type, str):
            try:
                type = TYPES[type]
            except KeyError:
                raise ValueError('unknown type: {!r}'.format(type))
        if name in self._params:
            raise Exception('duplicate parameter')
        self._params[name] = Param(type, default)
    def parse(self, x):
        result = {}
        for part in x.split(':') if x is not None else ():
            i = part.find('=')
            if i >= 0:
                name = part[:i]
                val = part[i+1:]
            else:
                name = part
                val = ''
            try:
                param = self._params[name]
            except KeyError:
                raise ConfigError('no such parameter: {!r}'.format(name))
            if name in result:
                raise ConfigError('duplicate parameter: {}'.format(name))
            if param.type is not None:
                try:
                    val = param.type(val)
                except ValueError as ex:
                    raise ConfigError('invalid value for {}: {!r}'.format(name, val))
            result[name] = val
        for key, param in self._params.items():
            result.setdefault(key, param.default)
        return result
