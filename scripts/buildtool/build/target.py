import os
import subprocess

def cpu_count():
    ncpu = None
    try:
        import multiprocessing
    except ImportError:
        pass
    else:
        ncpu = multiprocessing.cpu_count()
    if ncpu is None:
        if hasattr(os, "sysconf"):
            if 'SC_NPROCESSORS_ONLN' in os.sysconf_names:
                ncpu = os.sysconf('SC_NPROCESSORS_ONLN')
            elif 'NUMBER_OF_PROCESSORS' in os.environ:
                ncpu = int(os.environ['NUMBER_OF_PROCESSORS'])
    if ncpu is not None and ncpu >= 1:
        return ncpu
    return 1

class Build(object):
    """A build is a set of targets."""

    def __init__(self):
        self._targets = []

    def add(self, target):
        """Add a target to the build set."""
        self._targets.append(target)

    def _mkdirs(self, quiet=False):
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
        for target in self._targets:
            for path in target.outputs:
                mkdirp(os.path.dirname(path))

    def build(self):
        """Build all targets.

        Return True if successful, False if failed."
        """
        outputs = set(path for target in self._targets
                      for path in target.outputs)
        revdeps = {}
        depcounts = {}
        buildable = []
        unbuildable = 0
        for target in self._targets:
            ndeps = 0
            for path in target.inputs:
                if path not in outputs:
                    continue
                ndeps += 1
                try:
                    revdeps[path].append(target)
                except KeyError:
                    revdeps[path] = [target]
            depcounts[target] = ndeps
            if not ndeps:
                buildable.append(target)
            else:
                unbuildable += 1
        self._revdeps = revdeps
        self._depcounts = depcounts
        self._buildable = buildable
        self._unbuildable = unbuildable
        self._failures = 0
        self._successes = 0

        self._mkdirs()
        self.build1()

        print 'Targets: %d built, %d failed, %d skipped' % \
            (self._successes, self._failures, self._unbuildable)
        if self._failures or self._unbuildable:
            print 'Build failed'
            return False
        else:
            print 'Build succeeded'
            return True

    def build1(self):
        while self._buildable:
            target = self._buildable.pop(0)
            if target.build():
                self._successes += 1
                for path in target.outputs:
                    for target in self._revdeps[path]:
                        self._depcounts[target] -= 1
                        if not self._depcounts[target]:
                            self._unbuildable -= 1
                            self._buildable.append(target)
            else:
                self._failures += 1

class Target(object):
    """A target is a command line with dependencies.

    A target has inputs and outputs, which are lists of file paths.
    """

    def __init__(self, cmd, cwd=None, inputs=None, outputs=None,
                 quietmsg=None):
        self.cmd = cmd
        self.cwd = cwd
        self.inputs = inputs
        self.outputs = outputs
        self.quietmsg = quietmsg

    def build(self, quiet=True):
        """Build the target.

        Return True if successful, False on failure.
        """
        if quiet and self.quietmsg is not None:
            line = self.quietmsg
        elif isinstance(self.cmd, string):
            line = self.cmd
        else:
            line = ' '.join(self.cmd)
        print line
        proc = subprocess.Popen(self.cmd, cwd=self.cwd)
        status = proc.wait()
        return status == 0
