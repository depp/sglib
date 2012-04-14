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
    __slots__ = ['_filetargets', '_pseudotargets',
                 '_revdeps', '_depcount']
    def __init__(self):
        self._filetargets = {}
        self._pseudotargets = {}
        self._revdeps = {}
        self._depcount = {}

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
        inputs = set(target.input())
        self._depcount[target] = len(inputs)
        for t in set(target.input()):
            try:
                revdep = self._revdeps[t]
            except KeyError:
                revdep = [target]
                self._revdeps[t] = revdep
            else:
                revdep.append(target)

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

    def _rclosure(self, targets):
        """Compute the reverse closure of the given target set.

        The input should be a sequence of targets.  The output will be
        a set of targets.
        """
        x = set(targets)
        y = x
        while y:
            z = set()
            for t in y:
                for t2 in t.output():
                    try:
                        rd = self._revdeps[t2]
                    except KeyError:
                        pass
                    z.update(rd)
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

    def _build_make(self, buildlist, settings):
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
        buildlist.append(target.DepTarget('all', tnames))
        makefile = 'build/Makefile.tmp'
        self._mkdirs([Path(makefile)])
        ncpu = cpucount.cpucount()
        cmd = ['make', '-B', '-f', makefile]
        if ncpu > 1:
            cmd.append('-j%d' % ncpu)
        try:
            with open(makefile, 'wb') as f:
                f.write('all:\n')
                self._gen_gmake(buildlist, f, False, settings.VERBOSE)
            print 'make'
            proc = subprocess.Popen(cmd)
            status = proc.wait()
        finally:
            try:
                os.unlink(makefile)
            except OSError:
                pass
        return status == 0

    def _build_direct(self, buildlist, settings):
        """Build all targets in the given list directly."""
        buildlist = set(buildlist)
        if not buildlist:
            return True
        outputs = set()
        inputs = set()
        req_counts = dict(self._depcount)
        rev_dep = self._revdeps # don't modify
        queue = []
        for t in buildlist:
            i = list(t.input())
            if not i:
                queue.append(t)
            outputs.update(t.output())
            inputs.update(i)

        def post(t):
            """Mark a path or string as built and update the queue."""
            try:
                r = rev_dep[t]
            except KeyError:
                return
            for tt in r:
                if tt not in buildlist:
                    continue
                req_counts[tt] -= 1
                if not req_counts[tt]:
                    queue.append(tt)

        for t in inputs - outputs:
            post(t)

        queue.sort(key=tkey)

        self._mkdirs(outputs)

        success = 0
        failure = 0

        verbose = settings.VERBOSE
        while queue:
            t = queue.pop()
            if t.build(verbose):
                success += 1
                for tt in t.output():
                    post(tt)
            else:
                failure += 1

        skipped = len(buildlist) - (success + failure)
        print 'Targets: %d built, %d failed, %d skipped' % \
            (success, failure, skipped)
        return (not failure) and (not skipped)

    def build(self, targets, settings):
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
            # Divide targets into late / not late
            all_late = []
            for t in self.targetobjs():
                if t.late():
                    all_late.append(t)
            all_late = self._rclosure(all_late)
            late = buildset & all_late
            notlate = buildset - all_late
            del all_late
            del buildset

            # Divide not late targets into early / make
            early = set()
            for t in notlate:
                try:
                    t.write_rule
                except AttributeError:
                    early.add(t)
            try:
                t = self._pseudotargets['built-sources']
            except KeyError:
                pass
            else:
                early.update(self._closure([t]) & notlate)
            early = self._closure(early)
            make = notlate - early
            del notlate

            # Run all build phases
            if not self._build_direct(early, settings):
                return False
            if not self._build_make(make, settings):
                return False
            if not self._build_direct(late, settings):
                return False
            return True
        else:
            return self._build_direct(buildset, settings)

    def _gen_gmake(self, targets, f, generic, verbose):
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
            wr(f, generic, verbose)
            for i in t.input():
                if isinstance(i, str):
                    phony.add(i)
        if phony:
            f.write('.PHONY: ' + ' '.join(sorted(phony)))
            f.write('\n')

    def gen_gmake(self, targets, f):
        """Write the given targets as gmakefile rules."""
        targets = self._resolve(targets)
        buildable = list(self._closure(targets))
        self._gen_gmake(buildable, f, True, False)

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
