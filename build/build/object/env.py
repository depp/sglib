# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from build.shell import escape
from io import StringIO

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

class VarUniqueFlags(VarFlags):
    """An environment variable for program flags with removed duplicates."""

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

class VarAbsPathList(object):
    """An environment for absolute path lists."""

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
        return ' '.join(x)

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
    'FPATH':    VarAbsPathList,
    'FRAMEWORKS': VarUniqueFlags,
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

def dump_env(env, file, *, indent=''):
    for k, v in sorted(env.items()):
        print('{}{}: {}'.format(indent, k, VAR[k].show(v)), file=file)

class EnvModule(object):
    """Module which contains only a build environment."""
    __slots__ = ['env']
    def __init__(self, env):
        if not isinstance(env, dict):
            raise TypeError('environment must be dictionary')
        self.env = env
