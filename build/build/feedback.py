import sys

IN_FEEDBACK = False
class Feedback(object):
    """Context object for displaying "checking for..." feedback.

    On exit, this will print 'yes' for normal success, and 'no' for
    errors.  This can be overridden by using the 'write()' method
    first.
    """
    __slots__ = ['cfg', 'msg', 'active']
    def __init__(self, cfg, msg):
        self.cfg = cfg
        self.msg = msg
        self.active = False
    def __enter__(self):
        global IN_FEEDBACK
        if IN_FEEDBACK:
            return self
        self.active = True
        IN_FEEDBACK = True
        if self.cfg.verbosity >= 1:
            sys.stderr.write(self.msg)
            sys.stderr.flush()
        return self
    def write(self, msg, success=True):
        global IN_FEEDBACK
        if not self.active:
            return
        self.active = False
        IN_FEEDBACK = False
        if self.cfg.verbosity >= 1:
            sys.stderr.write(' {}\n'.format(msg))
    def __exit__(self, exc_type, exc_value, traceback):
        if not self.active:
            return
        if exc_value is None:
            self.write('yes')
        else:
            self.write('no')
