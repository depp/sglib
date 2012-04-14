import gen.build.target as target
from gen.env import Environment
from gen.path import Path
import platform

def add_sources(graph, proj, env, settings):
    pass

class Tarball(target.Target):
    __slots__ = ['_dest', '_src', '_env', '_prefix']
    def __init__(self, dest, src, env, prefix):
        if not isinstance(env, Environment):
            raise TypeError('env must be Environment')
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        src = list(src)
        for s in src:
            if not isinstance(s, Path):
                raise TypeError('src must be sequence of Path objects')
        self._dest = dest
        self._src = src
        self._env = env
        self._prefix = prefix

    def output(self):
        yield self._dest

    def input(self):
        return iter(self._src)

    def build(self, verbose):
        import cStringIO
        import subprocess
        env = self._env
        if not verbose:
            print 'TAR', self._dest.posix
        paths = [p.posix for p in self._src]
        paths.sort()
        io = cStringIO.StringIO()
        for p in paths:
            io.write(p)
            io.write('\n')
        slist = io.getvalue()
        del paths
        del io
        cmd = ['tar', '-Jc', '-f', self._dest.posix, '-T', '-']
        if self._prefix:
            cmd.extend(('--transform', 's,^,%s,' % self._prefix))
        if verbose:
            print ' '.join(cmd)
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
        out, err = proc.communicate(slist)
        status = proc.returncode
        return status == 0

    def late(self):
        return True

def add_targets(graph, proj, env, settings):
    if platform.system() in ('Linux', 'Darwin'):
        source_targets = graph.closure(['built-sources'])
        all_targets = graph.closure(['gmake', 'xcode'])
        paths = set()
        for t in all_targets - source_targets:
            for x in t.distinput():
                if isinstance(x, Path):
                    paths.add(x)
            for x in t.output():
                if isinstance(x, Path):
                    paths.add(x)
        for source in proj.sourcelist.sources():
            paths.add(source.relpath)
        nm = ('%s-%s-source' %
              (proj.info.PKG_FILENAME, proj.info.PKG_APP_VERSION))
        sp = Path('build/product', nm + '.tar.xz')
        graph.add(Tarball(sp, paths, env, nm + '/'))
        graph.add(target.DepTarget('source-xz', [sp]))
