# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import collections

class VarList(collections.Mapping):
    """Variable list.

    Tracks which variables are used and unused.
    """
    __slots__ = ['_vars', '_used']
    def __init__(self, vars=None):
        self._vars = {} if vars is None else dict(vars)
        self._used = set()
    def __len__(self):
        return len(self._vars)
    def __contains__(self, index):
        return index in self._vars
    def __iter__(self):
        return iter(self._vars)
    def __getitem__(self, index):
        v = self._vars[index]
        self._used.add(index)
        return v
    def __bool__(self):
        return bool(self._vars)
    def unused(self):
        """Iterate over unused variable names."""
        return iter(frozenset(self._vars).difference(self._used))
    @classmethod
    def parse(class_, vars):
        """Parse a list of NAME=VALUE strings into a VarList."""
        vardict = {}
        for var in vars:
            i = var.find('=')
            if i <= 0:
                raise ConfigError('invalid variable definition: {!r}'
                                  .format(var))
            vardict[var[:i]] = var[i+1:]
        return class_(vardict)
