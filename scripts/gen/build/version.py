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

    def contents(self):
        env = self._env
        return VERSION_TEMPL % (
            env.PKG_SG_VERSION, env.PKG_SG_COMMIT,
            env.PKG_APP_VERSION, env.PKG_APP_COMMIT,
        )

    def build(self):
        print 'VERSION', self._dest.posix
        data = self.contents()
        with open(self._dest.native, 'wb') as f:
            f.write(data)
        return True

    def write_rule(self, f, generic):
        """Write the makefile rule for this target to the given file."""
        f.write('%s:\n' % self._dest.posix)
        f.write("\trm -f $@\n")
        for line in self.contents().splitlines():
            f.write("\techo '%s' >> $@\n" % (line,))

def add_sources(graph, proj, userenv):
    g = proj.sourcelist.get_group('version', Path('.'))
    g.add(Path('version.c'), ())
    graph.add(VersionFile(Path('version.c'), userenv))

def add_targets(graph, proj, userenv):
    pass
