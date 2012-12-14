class ConfigError(Exception):
    """Project configuration failed."""

    __slots__ = ['reason', 'details']

    def __init__(self, reason, details=None):
        self.reason = reason
        self.details = details

def format_block(text):
    """Format a block to separate it in error messages."""
    a = []
    for line in text.splitlines():
        a.extend(('  | ', line, '\n'))
    return ''.join(a)
