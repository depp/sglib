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
                list(env.CPPFLAGS) + list(warn) + list(cflags)]

class LD(target.LD):
    """Link an executable.

    The 'types' parameter is an iterable of all the types of sources
    included.  It is used to choose between gcc and g++ for linking.
    """
    __slots__ = ['_dest', '_src', '_env', '_types']

    def __init__(self, dest, src, env, types):
        target.LD.__init__(self, dest, src, env)
        self._types = frozenset(types)

    def commands(self):
        env = self._env
        if 'cxx' in self._types:
            cc = env.CXX
        else:
            cc = env.CC
        return [[cc] + list(env.LDFLAGS) + ['-o', self._dest] +
                self._src + list(env.LIBS)]

_DEFAULT_CFLAGS = {
    'debug': '-O0',
    'release': '-O2',
}

def get_default_env(settings):
    """Get the default environment for a GCC-based build."""
    config_env = {
        'debug': Environment(CFLAGS='-O0', CXXFLAGS='-O0'),
        'release': Environment(CFLAGS='-O2', CXXFLAGS='-O2')
    }
    common_env = Environment(
        CC='gcc',
        CXX='g++',
        CWARN='-Wall -Wextra -Wpointer-arith -Wno-sign-compare ' \
            '-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes',
        CXXWARN='-Wall -Wextra -Wpointer-arith -Wno-sign-compare',
    )
    return Environment(common_env, config_env[settings.CONFIG])

def getmachine(env):
    """Get the name of the target machine."""
    m = shell.getoutput([env.CC, '-dumpmachine'] +
                        list(env.CPPFLAGS) + list(env.CFLAGS))
    i = m.find('-')
    if i < 0:
        raise Exception('unable to parse machine name: %r' % (m,))
    m = m[:i]
    if m == 'x86_64':
        return 'x64'
    if re.match('i\d86', m):
        return 'x86'
    if m == 'powerpc':
        return 'ppc'
    print >>sys.stderr, 'warning: unknown machine name: %r' % (m,)
    return m

