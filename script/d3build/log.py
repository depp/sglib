# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import io
import sys

class Tee(io.TextIOBase):
    """A file which redirects output to multiple files."""
    __slots__ = ['_files']
    def __init__(self, *files):
        super(Tee, self).__init__()
        self._files = files
    def writable(self):
        return True
    def write(self, s):
        for fp in self._files:
            fp.write(s)

_REDIRECTION = None

class Log(object):
    """A context object which copies output to a log file."""
    __slots__ = ['path', 'verbosity', 'console', 'logfile', 'tee']
    def __init__(self, path, verbosity):
        self.path = path
        self.verbosity = verbosity
    def __enter__(self):
        global _REDIRECTION
        if _REDIRECTION is not None:
            raise Exception('log file already active')
        self.console = sys.stderr
        self.logfile = open(self.path, 'w')
        self.tee = Tee(self.console, self.logfile)
        _REDIRECTION = self
    def __exit__(self, exc_type, exc_value, traceback):
        global _REDIRECTION
        _REDIRECTION = None
        self.logfile.close()
        del self.console
        del self.logfile
        del self.tee

def logfile(verbosity=1):
    """Get a log file for logging with the given verbosity level."""
    r = _REDIRECTION
    if r is None:
        return sys.stderr
    elif r.verbosity >= verbosity:
        return r.tee
    return r.logfile

_IN_FEEDBACK = False
class Feedback(object):
    """Context object for displaying "checking for..." feedback.

    On exit, this will print 'yes' for normal success, and 'no' for
    errors.  This can be overridden by using the 'write()' method
    first.
    """
    __slots__ = ['msg', 'active']
    def __init__(self, msg):
        self.msg = msg
        self.active = False
    @staticmethod
    def _files():
        r = _REDIRECTION
        if r is None:
            return sys.stderr, None
        if r.verbosity >= 1:
            return r.console, r.logfile
        return None, r.logfile
    def __enter__(self):
        global _IN_FEEDBACK
        if _IN_FEEDBACK:
            return self
        self.active = True
        _IN_FEEDBACK = True
        console, logfile = self._files()
        if console is not None:
            console.write(self.msg)
            console.flush()
        if logfile is not None:
            logfile.write(self.msg + '\n')
        return self
    def write(self, msg, success=True):
        global _IN_FEEDBACK
        if not self.active:
            return
        self.active = False
        _IN_FEEDBACK = False
        console, logfile = self._files()
        if console is not None:
            console.write(' {}\n'.format(msg))
        if logfile is not None:
            logfile.write('{} {}\n'.format(self.msg, msg))
    def __exit__(self, exc_type, exc_value, traceback):
        if not self.active:
            return
        if exc_value is None:
            self.write('yes')
        else:
            self.write('no')
