from gen.shell import escape
from gen.path import Path
import re
from io import StringIO
import posixpath
import sys

MK_SPECIAL = re.compile('[^-_.+/A-Za-z0-9]')
def mk_escape1(x):
    c = x.group(0)
    if c == ' ':
        return '\\ '
    raise ValueError('invalid character: {!r}'.format(c))
def mk_escape(x):
    if not isinstance(x, str):
        raise TypeError('expected string, got {!r}'.format(x))
    try:
        return MK_SPECIAL.sub(escape, x)
    except ValueError:
        raise ValueError('invalid character in {!r}'.format(x))

class Makefile(object):
    """GMake Makefile generator."""

    __slots__ = ['_rulefp', '_all', '_phony', '_opt_include',
                 '_qnames', '_qctr']

    def __init__(self):
        self._rulefp = StringIO()
        self._all = set()
        self._phony = set()
        self._opt_include = set()
        self._qnames = {}
        self._qctr = 0

    def _get_qctr(self, name):
        try:
            return self._qnames[name]
        except KeyError:
            n = self._qctr
            self._qctr = n + 1
            self._qnames[name] = n
            return n

    def add_rule(self, target, sources, cmds, qname=None):
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
        dirs = [x for x in dirs if x]
        if dirs:
            write('\t@mkdir -p {}\n'.format(
                  ' '.join(escape(d) for d in dirs)))
        first = True
        for cmd in cmds:
            write('\t')
            if qname is not None:
                write('$(QUIET{})'.format(self._get_qctr(
                    qname if first else '')))
                first = False
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
            fp.write('-include {}\n'.format(
                     ' '.join(mk_escape(x)
                              for x in sorted(self._opt_include))))
        fp.write('ifndef V\n')
        for k, v in sorted(self._qnames.items()):
            if k:
                fp.write("QUIET{} = @echo '    {}' $@;\n".format(v, k))
            else:
                fp.write("QUIET{} = @\n".format(v))
        fp.write('endif\n')
        fp.write(
            '.PHONY: all clean\n'
            'clean:\n'
            '\trm -rf build\n'
        )
        fp.write(self._rulefp.getvalue())

    def opt_include(self, path):
        self._opt_include.add(path.posix)

    def gen_regen(self, bcfg):
        """Generate commands to regenerate built sources."""
        from gen.config import CACHE_FILE, DEFAULT_ACTIONS
        import os.path
        spath = os.path.abspath(__file__)
        for i in range(3):
            spath = os.path.dirname(spath)
        spath = os.path.relpath(os.path.join(spath, 'init.py'))
        config_script = [sys.executable, spath]
        cache_file = Path(CACHE_FILE)
        self.add_rule(
            cache_file, bcfg.projectconfig.xmlfiles,
            [[sys.executable, spath, 'reconfig']],
            qname='Reconfigure')
        actions = bcfg.projectconfig.actions
        for action_name in DEFAULT_ACTIONS[bcfg.os]:
            target = actions[action_name][0]
            self.add_rule(
                target, [cache_file],
                [[sys.executable, spath, 'build', action_name]],
                qname='Regen')
            if target.posix != 'Makefile':
                self.add_default(target)
