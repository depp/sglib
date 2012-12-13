from gen.shell import escape
from gen.path import Path
import re
from cStringIO import StringIO
import posixpath

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

    __slots__ = ['_rulefp', '_all', '_phony', '_opt_include']

    def __init__(self):
        self._rulefp = StringIO()
        self._all = set()
        self._phony = set()
        self._opt_include = set()

    def add_rule(self, target, sources, cmds):
        """Add a rule to the makefile.

        The target should be a string, and the sources should be a
        list of strings.  The commands should be a list of commands,
        and each command should be a list of strings.  This handles
        escaping of all of the sources, targets, and commands.
        """
        write = self._rulefp.write
        if isinstance(target, Path):
            write(mk_escape(target.posix))
            dirs = [posixpath.dirname(target.posix)]
        else:
            write(mk_escape(target))
            dirs = []
            self._phony.add(target)
        write(':')
        for s in sources:
            if isinstance(s, Path):
                s = s.posix
            write(' ' + mk_escape(s))
        write('\n')
        if dirs:
            write('\t@mkdir -p %s\n' %
                  ' '.join(escape(d) for d in dirs))
        for cmd in cmds:
            write('\t')
            write(escape(cmd[0]))
            for arg in cmd[1:]:
                if isinstance(arg, Path):
                    arg = arg.posix
                write(' ' + escape(arg))
            write('\n')

    def add_default(self, target):
        """Make a target one of the default targets."""
        if isinstance(target, Path):
            self._all.add(target.posix)
        else:
            self._all.add(target)

    def write(self, fp):
        fp.write('all:')
        for target in sorted(self._all):
            fp.write(' ' + target)
        fp.write('\n')
        if self._opt_include:
            fp.write('-include %s\n' %
                     ' '.join(mk_escape(x)
                              for x in sorted(self._opt_include)))
        fp.write(
            '.PHONY: all clean\n'
            'clean:\n'
            '\trm -rf build\n'
        )
        fp.write(self._rulefp.getvalue())

    def opt_include(self, path):
        self._opt_include.add(path.posix)
