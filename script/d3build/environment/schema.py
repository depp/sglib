# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..shell import escape

class Schema(object):
    """A schema is a description of variables affecting a build system."""
    __slots__ = ['variables']

    def __init__(self):
        self.variables = {}

    def __getitem__(self, key):
        return self.variables[key]

    def __contains__(self, key):
        return key in self.variables

    def update(self, other):
        if not isinstance(other, Schema):
            raise TypeError('object must be a schema')
        self.variables.update(other.variables)

def _schema(name):
    def func(class_):
        def add_var(self, name, doc):
            self.variables[name] = class_(doc)
            return self
        setattr(Schema, name, add_var)
        return class_
    return func

class Var(object):
    """A build variable."""
    __slots__ = ['doc']

    def __init__(self, doc):
        self.doc = doc

@_schema('prog')
class VarProg(Var):
    """A build variable which stores the name or path of a program."""
    __slots__ = []

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

@_schema('flags')
class VarFlags(Var):
    """A build variable which stores command-line flags.."""
    __slots__ = []

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

@_schema('unique_flags')
class VarUniqueFlags(VarFlags):
    """A build variable which stores unique command-line flags."""
    __slots__ = []

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

@_schema('paths')
class VarPaths(Var):
    """A build variable which stores paths."""
    __slots__ = []

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

@_schema('defs')
class VarDefs(Var):
    """A build variable which stores preprocessor definitions."""
    __slots__ = []

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
