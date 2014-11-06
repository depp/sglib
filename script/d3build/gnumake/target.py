# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..target import BaseTarget
from ..error import UserError
from ..shell import escape
from .module import GnuMakeModule, cc_command, ld_command
from .schema import GnuMakeSchema
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
        # The schema for build variables.
        'schema',
        # The base build variables, a module.
        'base',

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

    def __init__(self, build, name):
        super(GnuMakeTarget, self).__init__()
        self.schema = GnuMakeSchema()
        self.base = GnuMakeModule(self.schema)
        self.base.add_variables({'CC': 'cc', 'CXX': 'c++'})
        base_vars = [('Debug', '-O0 -g'), ('Release', '-O')]
        for config, cflags in base_vars:
            cflags = cflags.split()
            var = {'CFLAGS': cflags, 'CXXFLAGS': cflags}
            self.base.add_variables(var, configs=[config])
        self._configure = [sys.executable, build.script]
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

    def module(self):
        return GnuMakeModule(self.schema).add_module(self.base)

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
        products = {variant: os.path.join('build', variant, 'products', name)
                    for variant in self.schema.variants}
        if not self._add_module(module):
            return products
        for variant, exepath in products.items():
            objects = []
            sourcetypes = set()
            for source in module.sources:
                if source.sourcetype in ('c', 'c++', 'objc', 'objc++'):
                    sourcetypes.add(source.sourcetype)
                    objects.append(self._compile(source, variant))
            self._add_rule(
                exepath, objects,
                [ld_command(
                    module.variables(), variant, exepath,
                    objects, sourcetypes)],
                qname='Link')
        return products

    def finalize(self):
        super(GnuMakeTarget, self).finalize()
        for source in self.generated_sources:
            if not source.is_regenerated:
                continue
            if source.is_regenerated_only:
                self._clean.add(source.target)
            if source.is_regenerated_always:
                deps = ['FORCE']
            else:
                deps = source.dependencies
            qname, cmds = source.rule()
            if cmds is None:
                cmds = [self._configure +
                        ['--action-regenerate', source.target]]
            self._add_rule(source.target, deps, cmds, qname=qname)
        with open('Makefile', 'w') as fp:
            self._write(fp)

    def _compile(self, source, variant):
        """Create a target that compiles a source file."""
        src = source.path
        assert not os.path.isabs(src)
        out = os.path.join(
            'build', variant, 'obj', os.path.splitext(src)[0])
        obj = out + '.o'
        variables = source.variables
        qname = BUILD_NAMES[source.sourcetype]
        if source.external:
            cmd = cc_command(
                variables, variant, obj, src,
                source.sourcetype, external=True)
        else:
            dep = out + '.d'
            cmd = cc_command(
                variables, variant, obj, src,
                source.sourcetype, depfile=dep)
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
