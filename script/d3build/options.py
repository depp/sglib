import argparse

class EnableFlag(object):
    """An flag which enables or disables optional features."""
    __slots__ = ['name', 'help', 'options', 'requirements', 'mandatory']

    def __init__(self, name, help, options=None, *,
                 require=None, mandatory=False):
        self.name = name
        self.help = help
        self.options = options
        self.requirements = [] if require is None else require
        self.mandatory = mandatory

    def add_argument(self, g):
        """Add the enable flag to an argument parser."""
        dest = 'flag:' + self.name
        if self.options is not None:
            options = list(self.options.keys())
            options.sort()
            if not self.mandatory:
                options.append('none')
            g.add_argument(
                '--enable-' + self.name,
                choices=options,
                dest=dest,
                default=None,
                help=self.help)
            if not self.mandatory:
                g.add_argument(
                    '--disable-' + self.name,
                    dest=dest,
                    action='store_const',
                    const='none',
                    default=None,
                    help=argparse.SUPPRESS)
        else:
            g.add_argument(
                '--enable-' + self.name,
                action='store_true',
                dest=dest,
                default=None,
                help=self.help)
            g.add_argument(
                '--disable-' + self.name,
                action='store_false',
                dest=dest,
                default=None,
                help=argparse.SUPPRESS)

    def get_value(self, args):
        """Get the flag value from an argument namespace."""
        return getattr(args, 'flag:' + self.name)
