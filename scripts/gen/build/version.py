from __future__ import with_statement
from gen.path import Path
import gen.build.target as target

VERSION_TEMPL = """\
/* Generated automatically by build system */
const char SG_SG_VERSION[] = "%s";
const char SG_SG_COMMIT[] = "%s";
const char SG_APP_VERSION[] = "%s";
const char SG_APP_COMMIT[] = "%s";
"""

class VersionFile(target.Target):
    __slots__ = ['_dest', '_info']
    def __init__(self, dest, info):
        self._dest = dest
        self._info = info

    def input(self):
        return iter(())

    def output(self):
        yield self._dest

    def contents(self):
        info = self._info
        return VERSION_TEMPL % (
            info.PKG_SG_VERSION, info.PKG_SG_COMMIT,
            info.PKG_APP_VERSION, info.PKG_APP_COMMIT,
        )

    def build(self, verbose):
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

def add_sources(graph, proj, env, settings):
    g = proj.sourcelist.get_group('Version', Path('.'))
    g.add(Path('version.c'), ())
    graph.add(VersionFile(Path('version.c'), proj.info))

def add_targets(graph, proj, env, settings):
    pass
