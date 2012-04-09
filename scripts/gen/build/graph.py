import gen.path as path
import os

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
                                     (t.posixpath,))
                self._filetargets[t] = target
            elif isinstance(t, str):
                if t in self._pseudotargets:
                    raise ValueError('duplicate target: %s' % (t,))
                self._pseudotargets[t] = target

    def _mkdirs(self, targets, quiet=False):
        """Make all necessary directories."""
        dirs = set()
        def mkdirp(path):
            if path in dirs:
                return
            if not os.path.isdir(path):
                par = os.path.dirname(path)
                if par:
                    mkdirp(par)
                if quiet:
                    print 'mkdir %s' % path
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

    def build(self, targets):
        """Build the given targets.

        Return True if successful, False if failed.
        """
        buildable = self._closure(self[t] for t in targets)
        inputs = set()
        outputs = set()
        req_counts = {}
        rev_dep = {}
        queue = []
        for t in buildable:
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

        skipped = len(buildable) - (success + failure)
        print 'Targets: %d built, %d failed, %d skipped' % \
            (success, failure, skipped)
        if failure or skipped:
            print 'Build failed'
            return False
        else:
            print 'Build succeeded'
            return True
