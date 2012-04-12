from __future__ import with_statement
import gen.atom as atom
import gen.path as path
import gen.build.target as target
import gen.cpucount as cpucount
import os
import sys
import platform
import subprocess

Path = path.Path

def tkey(t):
    for x in t.output():
        if isinstance(x, Path):
            x = '0' + x.posix
        else:
            x = '1' + x
        return (x, id(t))
    return ('', id(t))

class Graph(object):
    """A graph is a set of targets with dependency information.

    A graph can be executed, which builds a set of targets and all
    dependent targets.  A graph can also be exported as makefile
    rules, although not all targets can be preserved.
    """
    def __init__(self):
        self._filetargets = {}
        self._pseudotargets = {}

    def __getitem__(self, key):
        if isinstance(key, Path):
            return self._filetargets[key]
        elif isinstance(key, str):
            return self._pseudotargets[key]
        raise TypeError('invalid target name')

    def targets(self):
        """Iterate over all target names and paths."""
        for t in self._filetargets.iterkeys():
            yield t
        for t in self._pseudotargets.iterkeys():
            yield t

    def targetobjs(self):
        """Return a set of all targets."""
        s = set()
        s.update(self._filetargets.itervalues())
        s.update(self._pseudotargets.itervalues())
        return s

    def add(self, target):
        """Add a target to the build set.

        This will raise an exception if there is already a target in
        the graph with the same outputs.
        """
        has_output = False
        for t in target.output():
            has_output = True
            if isinstance(t, Path):
                if t in self._filetargets:
                    raise ValueError('duplicate target: %s' %
                                     (t.posix,))
                self._filetargets[t] = target
            elif isinstance(t, str):
                if t in self._pseudotargets:
                    raise ValueError('duplicate target: %s' % (t,))
                self._pseudotargets[t] = target

    def _mkdirs(self, targets, verbose=False):
        """Make all necessary directories."""
        dirs = set([''])
        def mkdirp(path):
            if path in dirs:
                return
            if not os.path.isdir(path):
                par = os.path.dirname(path)
                if par:
                    mkdirp(par)
                if verbose:
                    print 'mkdir', repr(path)
                os.mkdir(path)
            dirs.add(path)
        for t in targets:
            if isinstance(t, Path):
                mkdirp(os.path.dirname(t.native))

    def _closure(self, targets):
        """Compute the closure of the given target set.

        The input should be a sequence of targets.  The output will be
        a set of targets.
        """
        x = set(targets)
        y = x
        while y:
            z = set()
            for t in y:
                for t2 in t.input():
                    try:
                        tv = self[t2]
                    except KeyError:
                        pass
                    else:
                        z.add(tv)
            y = z - x
            x.update(z)
        return x

    def _resolve(self, targets):
        """Resolve targets as either pseudo-targets or file targets."""
        tset = set([None])
        ts = []
        unknown = []
        for t in targets:
            try:
                tt = self._pseudotargets[t]
            except KeyError:
                try:
                    tt = self._filetargets[Path(t)]
                except KeyError:
                    unknown.append(t)
                    tt = None
            if tt not in tset:
                ts.append(tt)
                tset.add(tt)
        if unknown:
            raise ValueError('error: unknown targets:', ' '.join(unknown))
        return ts

    def _build_make(self, buildlist, env):
        """Build all targets in the given list using Make."""
        buildlist = list(buildlist)
        for t in buildlist:
            if not isinstance(t, target.DepTarget):
                break
        else:
            return True
        tnames = []
        for t in buildlist:
            tnames.extend(t.output())
        buildlist.append(target.DepTarget('all', tnames, env))
        makefile = 'build/Makefile.tmp'
        self._mkdirs([Path(makefile)])
        ncpu = cpucount.cpucount()
        cmd = ['make', '-B', '-f', makefile]
        if ncpu > 1:
            cmd.append('-j%d' % ncpu)
        try:
            with open(makefile, 'wb') as f:
                f.write('all:\n')
                self._gen_gmake(buildlist, False, f)
            print 'make'
            proc = subprocess.Popen(cmd)
            status = proc.wait()
        finally:
            try:
                os.unlink(makefile)
            except OSError:
                pass
        return status == 0

    def _build_direct(self, buildlist, env):
        """Build all targets in the given list directly."""
        buildlist = list(buildlist)
        if not buildlist:
            return True
        outputs = set()
        inputs = set()
        req_counts = {}
        rev_dep = {}
        queue = []
        for t in buildlist:
            i = list(t.input())
            inputs.update(i)
            outputs.update(t.output())
            for tt in i:
                try:
                    r = rev_dep[tt]
                except KeyError:
                    r = [t]
                    rev_dep[tt] = r
                else:
                    r.append(t)
            req_counts[t] = len(i)
            if not i:
                queue.append(t)

        def post(t):
            """Mark a path or string as built and update the queue."""
            try:
                r = rev_dep[t]
            except KeyError:
                return
            for tt in r:
                req_counts[tt] -= 1
                if not req_counts[tt]:
                    queue.append(tt)

        for t in inputs - outputs:
            post(t)

        queue.sort(key=tkey)

        self._mkdirs(outputs)

        success = 0
        failure = 0

        while queue:
            t = queue.pop()
            if t.build():
                success += 1
                for tt in t.output():
                    post(tt)
            else:
                failure += 1

        skipped = len(buildlist) - (success + failure)
        print 'Targets: %d built, %d failed, %d skipped' % \
            (success, failure, skipped)
        return (not failure) and (not skipped)

    def build(self, targets, env):
        """Build the given targets.

        Return True if successful, False if failed.
        """
        targets = self._resolve(targets)
        buildset = self._closure(targets)

        outputs = set()
        for t in buildset:
            outputs.update(t.output())
        self._mkdirs(outputs)

        # Use Make if we can
        if platform.system() in ('Linux', 'Darwin'):
            directset = set()
            for t in buildset:
                try:
                    t.write_rule
                except AttributeError:
                    directset.add(t)
            try:
                t = self._pseudotargets['built-sources']
            except KeyError:
                pass
            else:
                directset.update(self._closure([t]) & buildset)
            directset = self._closure(directset)
            if not self._build_direct(directset, env):
                return False
            if not self._build_make(buildset - directset, env):
                return False
            return True
        else:
            return self._build_direct(buildset, env)

    def _gen_gmake(self, targets, generic, f):
        """Write the given targets as GNU make rules.

        No dependent targets will be written.  The 'generic' parameter
        must be true for generating a distributed Makefile, and may be
        false for generating a temporary Makefile.

        Generates a .PHONY rule if any pseudo-targets are included.
        """
        targs = list(targets)
        targs.sort(key=tkey)
        phony = set()
        for t in targs:
            try:
                wr = t.write_rule
            except AttributeError:
                raise TypeError('cannot generate make rule for %s' %
                                repr(t.__class__))
            wr(f, generic)
            for i in t.input():
                if isinstance(i, str):
                    phony.add(i)
        if phony:
            f.write('.PHONY: ' + ' '.join(sorted(phony)))

    def gen_gmake(self, targets, f):
        """Write the given targets as gmakefile rules."""
        targets = self._resolve(targets)
        buildable = list(self._closure(targets))
        self._gen_gmake(buildable, True, f)

    def platform_built_sources(self, proj, platform):
        """Get the built sources for the given platform."""
        for source in proj.sourcelist.sources():
            if source.relpath not in self._filetargets:
                continue
            pas = set(source.atoms) & atom.PLATFORMS
            if not pas or platform in pas:
                yield source.relpath

    def closure(self, targets):
        """Compute the closure of the given target names.

        Returns a new set of target objects.  The input is a sequence
        of target names (paths or strings).
        """
        return self._closure(self._resolve(targets))
