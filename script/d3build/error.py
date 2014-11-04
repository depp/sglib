# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .log import logfile

class ConfigError(Exception):
    """Configuration failed."""
    __slots__ = ['reason', 'details', 'childerrors']

    def __init__(self, reason, *, details=None, childerrors=None):
        self.reason = reason
        self.details = details
        self.childerrors = childerrors

    def write(self, fp, indent=''):
        fp.write('{}error: {}\n'.format(indent, self.reason))
        if self.details is not None:
            for line in self.details.splitlines():
                if line:
                    fp.write('{}  {}\n'.format(indent, line))
                else:
                    fp.write('\n')
        if self.childerrors is not None:
            indent = indent + '  '
            for error in self.childerrors:
                error.write(fp, indent)

class UserError(Exception):
    """Configuration failed due to user error."""

def format_block(text):
    """Format a block to separate it in error messages."""
    a = []
    for line in text.splitlines():
        a.extend(('  | ', line, '\n'))
    return ''.join(a)
