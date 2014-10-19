# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import build.config as config

IN_FEEDBACK = False
class Feedback(object):
    """Context object for displaying "checking for..." feedback.

    On exit, this will print 'yes' for normal success, and 'no' for
    errors.  This can be overridden by using the 'write()' method
    first.
    """
    __slots__ = ['env', 'msg', 'active']
    def __init__(self, env, msg):
        self.env = env
        self.msg = msg
        self.active = False
    def __enter__(self):
        global IN_FEEDBACK
        if IN_FEEDBACK:
            return self
        self.active = True
        IN_FEEDBACK = True
        if self.env.verbosity >= 1:
            self.env.console.write(self.msg)
            self.env.console.flush()
        self.env.logfile().write(self.msg + '\n')
        return self
    def write(self, msg, success=True):
        global IN_FEEDBACK
        if not self.active:
            return
        self.active = False
        IN_FEEDBACK = False
        if self.env.verbosity >= 1:
            self.env.console.write(' {}\n'.format(msg))
        self.env.logfile().write('{} {}\n'.format(self.msg, msg))
    def __exit__(self, exc_type, exc_value, traceback):
        if not self.active:
            return
        if exc_value is None:
            self.write('yes')
        else:
            self.write('no')
