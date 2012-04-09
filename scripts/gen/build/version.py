from __future__ import with_statement
from gen.path import Path

VERSION_TEMPL = """\
/* Generated automatically by build system */
const char SG_SG_VERSION[] = "%s";
const char SG_SG_COMMIT[] = "%s";
const char SG_APP_VERSION[] = "%s";
const char SG_APP_COMMIT[] = "%s";
"""

class VersionFile(object):
    __slots__ = ['_dest', '_env']
    def __init__(self, dest, env):
        self._dest = dest
        self._env = env

    def input(self):
        return iter(())

    def output(self):
        yield self._dest

    def build(self):
        print 'VERSION', self._dest.posix
        env = self._env
        data = VERSION_TEMPL % (
            env.PKG_SG_VERSION, env.PKG_SG_COMMIT,
            env.PKG_APP_VERSION, env.PKG_APP_COMMIT,
        )
        with open(self._dest.native, 'wb') as f:
            f.write(data)
        return True

def add_targets(graph, proj, env):
    graph.add(VersionFile(Path('version.c'), env))
