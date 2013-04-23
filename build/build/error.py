class Error(Exception):
    """Project configuration failed."""

    __slots__ = ['reason', 'details', 'suberrors', 'loc']

    def __init__(self, reason, details=None, suberrors=None, loc=None):
        self.reason = reason
        self.details = details
        self.suberrors = suberrors
        self.loc = loc

    def write(self, fp, indent=''):
        if self.loc is None:
            fp.write('{}error: {}\n'.format(indent, self.reason))
        else:
            fp.write('{}{}: error: {}\n'
                     .format(indent, self.loc, self.reason))
        if self.details is not None:
            for line in self.details.splitlines():
                if line:
                    fp.write('{}  {}\n'.format(indent, line))
                else:
                    fp.write('\n')
        if self.suberrors is not None:
            indent = indent + '  '
            for error in self.suberrors:
                error.write(fp, indent)

class ProjectError(Error):
    """Error in project."""
    __slots__ = []

class ConfigError(Error):
    """Error in build environment."""
    __slots__ = []

def format_block(text):
    """Format a block to separate it in error messages."""
    a = []
    for line in text.splitlines():
        a.extend(('  | ', line, '\n'))
    return ''.join(a)
