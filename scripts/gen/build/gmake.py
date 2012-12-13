from gen.shell import escape
import re
from cStringIO import StringIO

MK_SPECIAL = re.compile('[^-_.+/A-Za-z0-9]')
def mk_escape1(x):
    c = x.group(0)
    if c == ' ':
        return '\\ '
    raise ValueError('invalid character: %r' % (c,))
def mk_escape(x):
    if not isinstance(x, basestring):
        raise TypeError('expected string, got %s' % repr(x))
    try:
        return MK_SPECIAL.sub(escape, x)
    except ValueError:
        raise ValueError('invalid character in %r' % (x,))

class Makefile(object):
    """GMake Makefile generator."""

    __slots__ = ['_rulefp', '_all']

    def __init__(self):
        self._rulefp = StringIO()
        self._all = set()

    def add_rule(self, target, sources, cmds):
        """Add a rule to the makefile.

        The target should be a string, and the sources should be a
        list of strings.  The commands should be a list of commands,
        and each command should be a list of strings.  This handles
        escaping of all of the sources, targets, and commands.
        """
        write = self._rulefp.write
        write('%s:' % mk_escape(target))
        for s in sources:
            write(' %s' % mk_escape(s))
        write('\n')
        for cmd in cmds:
            write('\t%s\n' % ' '.join(escape(x) for x in cmd))

    def add_default(self, target):
        """Make a target one of the default targets."""
        self._all.add(target)

    def write(self, fp):
        fp.write('all:')
        for target in sorted(self._all):
            fp.write(' ' + target)
        fp.write('\n')
        fp.write(
            '.PHONY: all clean\n'
            'clean:\n'
            '\trm -f build\n')
        fp.write(self._rulefp.getvalue())
