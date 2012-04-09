"""Common builders for *nix environments.

Includes compiler and linker targets for GCC-based systems.
"""

import gen.path as path
import gen.shell as shell
import gen.build.target as target
from gen.env import Environment
import os
import shutil
import re

def buildline(cmd, target, tag):
    if tag is None:
        return '%s %s' % (cmd, target)
    else:
        return '[%s] %s %s' % (tag, cmd, target)

class CC(target.CC):
    """Compile a C, C++, or Objective C file."""
    __slots__ = ['_dest', '_src', '_env', '_sourcetype']
    def commands(self):
        env = self._env
        if self._sourcetype in ('c', 'm'):
            cc = env.CC
            cflags = env.CFLAGS
            warn = env.CWARN
        elif self._sourcetype in ('cxx', 'mm'):
            cc = env.CXX
            cflags = env.CXXFLAGS
            warn = env.CXXWARN
        else:
            assert False
        return [[cc, '-o', self._dest, '-c', self._src] +
                [('-I', p) for p in env.CPPPATH] +
                env.CPPFLAGS + warn + cflags]

class LD(target.LD):
    """Link an executable."""
    __slots__ = ['_dest', '_src', '_env']
    def commands(self):
        env = self._env
        cc = env.CXX
        return [[cc] + env.LDFLAGS + ['-o', self._dest] +
                self._src + env.LIBS]

_DEFAULT_CFLAGS = {
    'debug': '-O0',
    'release': '-O2',
}

def get_user_env(env):
    """Get the default user environment for a GCC-based build.

    The input is the user-specified environment.  The default values
    will be filled in, but any value specified by the user will be
    untouched.
    """
    config = env.CONFIG
    try:
        cflags = _DEFAULT_CFLAGS[config]
    except KeyError:
        raise ValueError('unknown configuration: %r' % (config,))
    e = Environment(
        CC='gcc',
        CXX='g++',
        CPPFLAGS='',
        CFLAGS=cflags,
        CXXFLAGS=cflags,
        CWARN='-Wall -Wextra -Wpointer-arith -Wno-sign-compare ' \
            '-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes',
        CXXWARN='-Wall -Wextra -Wpointer-arith -Wno-sign-compare',
        LDFLAGS='',
        LIBS='',
    )
    e.override(env)
    return e
