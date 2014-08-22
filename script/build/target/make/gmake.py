# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from build.error import ConfigError
from build.object import GeneratedSource
from build.shell import escape
from build.path import Path
import re
from io import StringIO
import sys
import collections

Raw = collections.namedtuple('Raw', 'value')

BUILD_NAMES = {
    'c': 'C',
    'c++': 'C++',
    'objc': 'ObjC',
    'objc++': 'ObjC++',
}

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
        return MK_SPECIAL.sub(mk_escape1, x)
    except ValueError:
        raise ValueError('invalid character in {!r}'.format(x))

class GenMakefile(GeneratedSource):
    __slots__ = ['makefile']
    is_regenerated = False

    def write(self, fp, cfg):
        self.makefile.write(fp)

class Makefile(object):
    """GMake Makefile generator."""

    __slots__ = ['_rulefp', '_mrulefp', '_all', '_phony', '_opt_include',
                 '_qnames', '_qctr', '_cfg', '_clean']

    def __init__(self, cfg):
        self._rulefp = StringIO()
        self._mrulefp = StringIO()
        self._all = set()
        self._phony = set(['all', 'clean', 'FORCE'])
        self._opt_include = set()
        self._qnames = {}
        self._qctr = 0
        self._cfg = cfg
        self._clean = set()

    def add_build(self, build, makepath):
        target = self._cfg.target
        for btarget in build.targets():
            target_type = btarget.target_type
            try:
                func = getattr(target, 'make_' + target_type)
            except AttributeError:
                raise ConfigError('unsupported target type: {}'
                                  .format(target_type))
            func(self, build, btarget)
        target.make_sources(self, build)
        for gensrc in build.generated_sources():
            if not gensrc.is_regenerated:
                continue
            try:
                func = gensrc.make_self
            except AttributeError:
                deps = tuple(gensrc.deps)
                if gensrc.is_regenerated_always:
                    deps = deps + ('FORCE',)
                self.add_rule(
                    gensrc.target, deps,
                    [[sys.executable, sys.argv[0], 'regen', gensrc.target]],
                    qname='Regen')
            else:
                func(self, build)
        self.add_rule(
            makepath, build.proj.files,
            [[sys.executable, sys.argv[0], 'reconfig']])
        build.add(None, GenMakefile(
            target=makepath, makefile=self))

    def _get_qctr(self, name):
        try:
            return self._qnames[name]
        except KeyError:
            n = self._qctr
            self._qctr = n + 1
            self._qnames[name] = n
            return n

    def add_rule(self, target, sources, cmds, *,
                 qname=None, phony=None, srctype=None):
        """Add a rule to the makefile.

        The target should be a string or path, and the sources should
        be a list of strings or paths.  The commands should be a list
        of commands, and each command should be a list of arguments.
        Arguments are strings, paths, or tuples consisting of strings
        and paths.  This handles escaping of all of the sources,
        targets, and commands.
        """
        if qname is None:
            qname = BUILD_NAMES.get(srctype)
        convert_path = self._cfg.target_path
        if target.basename() == 'Makefile':
            write = self._mrulefp.write
        else:
            write = self._rulefp.write
        if isinstance(target, Path):
            dirs = [convert_path(target.dirname())]
            target = convert_path(target)
            default_phony = False
        else:
            dirs = []
            default_phony = True
        if (default_phony if phony is None else phony):
            self._phony.add(target)
        write(mk_escape(target))
        write(':')
        for s in sources:
            if isinstance(s, Path):
                s = convert_path(s)
            write(' ' + mk_escape(s))
        write('\n')
        dirs = [x for x in dirs if x != '.']
        if dirs:
            write('\t@mkdir -p {}\n'.format(
                  ' '.join(escape(d) for d in dirs)))
        first = True
        for cmd in cmds:
            write('\t')
            if isinstance(cmd, Raw):
                write(cmd.value)
                write('\n')
                continue
            if qname is not None:
                write('$(QUIET{})'.format(self._get_qctr(
                    qname if first else '')))
                first = False
            write(escape(cmd[0]))
            for arg in cmd[1:]:
                if isinstance(arg, str):
                    pass
                elif isinstance(arg, Path):
                    arg = convert_path(arg)
                elif isinstance(arg, tuple):
                    parts = []
                    for part in arg:
                        if isinstance(part, str):
                            parts.append(part)
                        elif isinstance(part, Path):
                            parts.append(convert_path(part))
                        else:
                            raise TypeError('invalid command type')
                    arg = ''.join(parts)
                else:
                    raise TypeError('invalid command type')
                write(' ' + escape(arg))
            write('\n')

    def add_default(self, *targets):
        """Make targets default targets."""
        for target in targets:
            if isinstance(target, Path):
                self._all.add(self._cfg.target_path(target))
            else:
                self._all.add(target)

    def add_clean(self, *targets):
        """Add files to remove when cleaning."""
        for target in targets:
            if not isinstance(target, Path):
                raise TypeError('expected path')
            if target.base == 'builddir':
                self._clean.add(self._cfg.target_path(target))
            else:
                self._cfg.warn('not cleaning non-path: {!r}'.format(target))

    def write(self, fp):
        fp.write('all: {}\n'.format(' '.join(
            mk_escape(target) for target in self._all)))
        fp.write(self._mrulefp.getvalue())
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
        fp.write('.PHONY: {}'.format(
            ' '.join(mk_escape(x) for x in sorted(self._phony))) + '\n')
        fp.write(
            'clean:\n'
            '\trm -rf {}\n'.format(' '.join(sorted(self._clean)))
        )
        fp.write('FORCE:\n')
        fp.write(self._rulefp.getvalue())

    def opt_include(self, *paths):
        for path in paths:
            self._opt_include.add(self._cfg.target_path(path))
