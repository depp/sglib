from .nix import NixEnvironment, cc_command, ld_command
from .variable import BuildVariables
from ..error import UserError
from ..shell import escape
import collections
import io
import os
import re

_escape = escape
def escape(x):
    try:
        return _escape(x)
    except:
        print(repr(x))
        raise

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

# A phony makefile target
Phony = collections.namedtuple('Phony', 'name')

class GnuMakeEnvironment(NixEnvironment):
    """Environment targeting the GNU Make build system."""
    __slots__ = [
        '_rulefp',      # StringIO with most rules
        '_mrulefp',     # StringIO with rules that generate makefiles
        '_all',         # What to build by default
        '_clean',       # What to clean
        '_phony',       # List of phony rules
        '_optinclude',  # Optional makefiles to include
        '_qnames',      # Map of quiet rule names
    ]

    def __init__(self, cfg):
        super(GnuMakeEnvironment, self).__init__(cfg)
        self._rulefp = io.StringIO()
        self._mrulefp = io.StringIO()
        self._all = set()
        self._clean = {'build'}
        self._phony = {'all', 'clean', 'FORCE'}
        self._optinclude = set()
        self._qnames = {}

    def target_executable(self, *, name, module, uuid=None):
        """Create an executable target."""
        cm = module.flatten(self)

        objects = []
        sourcetypes = set()
        for source in cm.sources:
            if source.sourcetype in ('c', 'c++', 'objc', 'objc++'):
                objects.append(self._compile(source))

        varset = BuildVariables.merge([self.base_vars] + cm.varsets)
        varset.dump()

        exepath = os.path.join('build', 'exe', name)
        debugpath = os.path.join('build', 'products', name + '.dbg')
        productpath = os.path.join('build', 'products', name)

        self._add_rule(
            exepath, objects,
            [ld_command(varset, exepath, objects, sourcetypes,
                        resolve_path=self._resolve_path)],
            qname='Link')
        self._add_rule(
            debugpath, [exepath],
            [['objcopy', '--only-keep-debug', exepath, debugpath],
             ['chmod', '-x', debugpath]],
            qname='ObjCopy')
        self._add_rule(
            productpath, [exepath, debugpath],
            [['objcopy', '--strip-unneeded',
              '--add-gnu-debuglink=' + debugpath, exepath, productpath]],
            qname='ObjCopy')

        self._all.add(productpath)

    def target_application_bundle(self, name, module, info_plist):
        """Create an OS X application bundle target."""

    def finalize(self):
        with open('Makefile', 'w') as fp:
            self._write(fp)

    def _compile(self, source):
        """Create a target that compiles a source file."""
        src = self._resolve_path(source.path)
        out = os.path.join(
            'build', 'obj', os.path.splitext(src)[0])
        varset = BuildVariables.merge([self.base_vars] + source.varsets)
        obj = out + '.o'
        qname = BUILD_NAMES[source.sourcetype]
        if source.external:
            cmd = cc_command(varset, obj, src, source.sourcetype,
                             external=True, resolve_path=self._resolve_path)
        else:
            dep = out + '.d'
            cmd = cc_command(varset, obj, src, source.sourcetype,
                             depfile=dep, resolve_path=self._resolve_path)
            self._optinclude.add(dep)
        self._add_rule(obj, [source.path], [cmd], qname=qname)
        return obj

    def _resolve_path(self, path):
        result = os.path.relpath(path)
        if result.startswith('../'):
            raise UserError('path outside root: {}'.format(path))
        return result

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
        if isinstance(target, Phony):
            target = target.name
            self._phony.add(target)
            dirpath = None
        else:
            if os.path.basename == 'Makefile':
                write = self._mrulefp.write
            dirpath = os.path.dirname(target)
            target = self._resolve_path(target)

        write(mk_escape(target))
        write(':')
        for source in sources:
            if isinstance(source, Phony):
                source = source.name
            else:
                source = self._resolve_path(source)
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
        fp.write(self._mrulefp.getvalue())
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
