from __future__ import with_statement
import os
import subprocess
import shutil

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

    def build(self, obj):
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
            for path in target.outputs:
                if path not in revdeps:
                    revdeps[path] = []
        self._revdeps = revdeps
        self._depcounts = depcounts
        self._buildable = buildable
        self._unbuildable = unbuildable
        self._failures = 0
        self._successes = 0

        quiet = not obj.opts.verbose
        self._mkdirs()
        self.build1(quiet)

        print 'Targets: %d built, %d failed, %d skipped' % \
            (self._successes, self._failures, self._unbuildable)
        if self._failures or self._unbuildable:
            print 'Build failed'
            return False
        else:
            print 'Build succeeded'
            return True

    def build1(self, quiet):
        while self._buildable:
            target = self._buildable.pop(0)
            if target.build(quiet):
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

    def build(self, quiet):
        """Build the target.

        Return True if successful, False on failure.
        """
        if quiet and self.name is not None:
            line = '%s %s' % (self.name, ' '.join(self.outputs))
        elif isinstance(self.cmd, str):
            line = self.cmd
        else:
            line = ' '.join(self.cmd)
        print line
        return self.run()

class Command(Target):
    """A command-line target."""

    def __init__(self, cmd, cwd=None, inputs=None, outputs=None,
                 name=None, pre=None, post=None):
        self.cmd = cmd
        self.cwd = cwd
        self.inputs = inputs
        self.outputs = outputs
        self.name = name
        self.pre = pre
        self.post = post
    
    def run(self):
        if self.pre is not None:
            if not self.pre():
                return False
        proc = subprocess.Popen(self.cmd, cwd=self.cwd)
        status = proc.wait()
        if status != 0:
            return False
        if self.post is not None:
            if not self.post():
                return False
        return True

class CopyFile(Target):
    """A target which copies a file."""

    def __init__(self, dest, src):
        self.outputs = [dest]
        self.inputs = [src]
        self.src = src
        self.dest = dest
        self.name = 'COPY'

    def run(self):
        shutil.copyfile(self.src, self.dest)
        return True

class StaticFile(Target):
    """A target which creates a file from a string."""

    def __init__(self, dest, contents):
        self.outputs = [dest]
        self.inputs = []
        self.dest = dest
        self.contents = contents
        self.name = 'FILE'

    def run(self):
        with open(self.dest, 'wb') as f:
            f.write(self.contents)
        return True
