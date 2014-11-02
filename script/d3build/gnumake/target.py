# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..target import BaseTarget
from ..error import UserError
from ..shell import escape
import io
import os
import re
import sys

# What to print out when compiling various source types
BUILD_NAMES = {
    'c': 'C',
    'c++': 'C++',
    'objc': 'ObjC',
    'objc++': 'ObjC++',
}

# Escape special characters in makefiles
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

class GnuMakeTarget(BaseTarget):
    """Environment targeting the GNU Make build system."""
    __slots__ = [
        # First arguments for running the configuration script.
        '_configure',
        # StringIO with the rules.
        '_rulefp',
        # Set of default targets.
        '_all',
        # Set of objects to clean.
        '_clean',
        # Set of phony rules (e.g. 'clean', 'all').
        '_phony',
        # Optional makefiles to include.
        '_optinclude',
        # Map from text printed to quiet rule variables.
        '_qnames',
    ]

    def __init__(self, name, script, config, env):
        super(GnuMakeTarget, self).__init__(name, script, config, env)
        self._configure = [sys.executable, script]
        self._rulefp = io.StringIO()
        self._all = set()
        self._clean = {'build'}
        self._phony = {'all', 'clean', 'FORCE', 'config'}
        self._optinclude = set()
        self._qnames = {}

    @property
    def run_srcroot(self):
        """The path root of the source tree, at runtime."""
        return '.'

    def add_generated_source(self, source):
        """Add a generated source to the build system.

        Returns the path to the generated source.
        """
        super(GnuMakeTarget, self).add_generated_source(source)
        if source.is_regenerated_only:
            self._clean.add(source.target)
        if source.is_regenerated_always:
            deps = ['FORCE']
        else:
            deps = source.dependencies
        qname, cmds = source.rule()
        if cmds is None:
            cmds = [self._configure + ['--action-regenerate', source.target]]
        self._add_rule(source.target, deps, cmds, qname=qname)
        return source.target

    def add_default(self, target):
        """Set a target to be a default target."""
        super(GnuMakeTarget, self).add_default(target)
        self._all.add(target)

    def add_alias(self, target, deps):
        """Create a target which triggers other targets."""
        self._phony.add(target)
        self._add_rule(target, deps, [])

    def add_executable(self, *, name, module, uuid=None, arguments=[]):
        """Create an executable target.

        Returns the path to the executable.
        """
        if module is None:
            return ''

        varset = self.env.schema.merge([self.env.base] + module.varsets)

        paths = {}
        for config in self.env.configs:
            objects = []
            sourcetypes = set()
            for source in module.sources:
                if source.sourcetype in ('c', 'c++', 'objc', 'objc++'):
                    sourcetypes.add(source.sourcetype)
                    objects.append(self._compile(source, config))

            exepath = os.path.join('build', config, 'products', name)
            paths[config] = exepath

            self._add_rule(
                exepath, objects,
                [self.env.ld_command(
                    config, varset, exepath, objects, sourcetypes)],
                qname='Link')

        return paths

    def finalize(self):
        super(GnuMakeTarget, self).finalize()
        with open('Makefile', 'w') as fp:
            self._write(fp)

    def _compile(self, source, config):
        """Create a target that compiles a source file."""
        src = source.path
        assert not os.path.isabs(src)
        out = os.path.join(
            'build', config, 'obj', os.path.splitext(src)[0])
        varset = self.env.schema.merge([self.env.base] + source.varsets)
        obj = out + '.o'
        qname = BUILD_NAMES[source.sourcetype]
        if source.external:
            cmd = self.env.cc_command(
                config, varset, obj, src, source.sourcetype, external=True)
        else:
            dep = out + '.d'
            cmd = self.env.cc_command(
                config, varset, obj, src, source.sourcetype, depfile=dep)
            self._optinclude.add(dep)
        self._add_rule(obj, [source.path], [cmd], qname=qname)
        return obj

    def _get_qname(self, text):
        try:
            return self._qnames[text]
        except KeyError:
            n = len(self._qnames)
            self._qnames[text] = n
            return n

    def _add_rule(self, target, sources, cmds, *, qname=None):
        """Add a rule to the makefile.

        The commands should be a list of commands, and each command is
        a list of arguments.
        """
        write = self._rulefp.write
        dirpath = os.path.dirname(target)

        write(mk_escape(target))
        write(':')
        for source in sources:
            write(' ')
            write(mk_escape(source))
        write('\n')

        if dirpath:
            write('\t@mkdir -p {}\n'.format(escape(dirpath)))

        for n, cmd in enumerate(cmds):
            write('\t')
            if qname is not None:
                write('$(QUIET{})'.format(
                    self._get_qname('' if n else qname)))
            for m, arg in enumerate(cmd):
                if m:
                    write(' ')
                write(escape(arg))
            write('\n')

    def _write(self, fp):
        """Write the makefile contents to a file."""
        fp.write('all: {}\n'.format(' '.join(
            mk_escape(target) for target in sorted(self._all))))
        fp.write(
            'Makefile: {script}\n\t{python} {script} --action-reconfigure\n'
            'config:\n\t{python} {script} --action-reconfigure\n'
            .format(python=escape(self._configure[0]),
                    script=escape(self._configure[1])))
        if self._optinclude:
            fp.write('-include {}\n'.format(
                ' '.join(mk_escape(x)
                         for x in sorted(self._optinclude))))

        fp.write('ifndef V\n')
        for k, v in sorted(self._qnames.items()):
            if k:
                fp.write("QUIET{} = @echo '    {}' $@;\n".format(v, k))
            else:
                fp.write("QUIET{} = @\n".format(v))
        fp.write('endif\n')

        fp.write('.PHONY: {}\n'.format(
            ' '.join(mk_escape(x) for x in sorted(self._phony))))
        fp.write('clean:\n')
        if self._clean:
            fp.write('\trm -rf {}\n'.format(
                ' '.join(escape(x) for x in sorted(self._clean))))
        fp.write('FORCE:\n')
        fp.write(self._rulefp.getvalue())
