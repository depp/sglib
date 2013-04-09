from . import nix
from . import env
from . import gensource
from build.shell import escape
from build.path import Path
import re
from io import StringIO
import sys

BUILD_NAMES = {
    'c': 'C',
    'c++': 'C++',
    'objc': 'ObjC',
    'objc++': 'ObjC++',
}

class Target(nix.Target):
    __slots__ = []

    def build_exe(self, makefile, build, module):
        source = build.modules[module.source]
        filename = module.filename

        objdir = Path('/build/obj', 'builddir')
        exedir = Path('/build/exe', 'builddir')
        proddir = Path('/build/products', 'builddir')

        objs = []
        src_types = set()
        for src in source.sources:
            if src.type in ('c', 'c++'):
                src_types.add(src.type)
                opath = objdir.join(src.path.to_posix()).withext('.o')
                objs.append(opath)
            elif src.type in ('header',):
                pass
            else:
                print('warning: ignored source file: {}'
                      .format(src.path), file=sys.stderr)

        build_env = [self.env]
        for req in source.deps:
            build_env.append(build.modules[req].env)
        build_env = env.merge_env(build_env)

        exepath = exedir.join(filename)
        debugpath = proddir.join(filename + '.dbg')
        prodpath = proddir.join(filename)

        makefile.add_rule(
            exepath, objs,
            [nix.ld_cmd(build_env, exepath, objs, src_types)],
            'LD')
        makefile.add_rule(
            debugpath, [exepath],
            [['objcopy', '--only-keep-debug', exepath, debugpath],
             ['chmod', '-x', debugpath]],
            'ObjCopy')
        makefile.add_rule(
            prodpath, [exepath, debugpath],
            [['objcopy', '--strip-unneeded',
              ('--add-gnu-debuglink=', debugpath),
              exepath, prodpath]],
            'ObjCopy')

        makefile.add_default(prodpath, debugpath)

    def build_sources(self, makefile, build):
        objdir = Path('/build/obj', 'builddir')
        for src in build.sources.values():
            if src.type in ('c', 'c++'):
                opath = objdir.join(src.path.to_posix()).withext('.o')
                dpath = opath.withext('.d')
                build_env = [self.env]
                for req in src.modules:
                    build_env.append(build.modules[req].env)
                build_env.append({'CPPPATH': src.header_paths})
                build_env = env.merge_env(build_env)
                makefile.add_rule(
                    opath, [src.path],
                    [nix.cc_cmd(build_env, opath, src.path, src.type,
                                depfile=dpath, external=src.external)],
                    BUILD_NAMES.get(src.type))
                makefile.opt_include(dpath)

    def gen_build(self, proj):
        build = super().gen_build(proj)
        makefile = Makefile(proj)
        makepath = Path('/Makefile', 'builddir')

        makefile.add_rule(
            makepath, proj.files,
            [[sys.executable, sys.argv[0], 'reconfig']])
            

        self.build_sources(makefile, build)
        for target in build.targets:
            if isinstance(target, nix.ExeModule):
                self.build_exe(makefile, build, target)

        build.add(GenMakefile(None, makepath, makefile))
        return build

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

class GenMakefile(gensource.GeneratedSource):
    __slots__ = ['makefile']

    def __init__(self, name, target, makefile):
        super(GenMakefile, self).__init__(name, target)
        self.makefile = makefile

    @property
    def deps(self):
        return ()

    def write(self, fp):
        self.makefile.write(fp)

class Makefile(object):
    """GMake Makefile generator."""

    __slots__ = ['_rulefp', '_mrulefp', '_all', '_phony', '_opt_include',
                 '_qnames', '_qctr', '_proj']

    def __init__(self, proj):
        self._rulefp = StringIO()
        self._mrulefp = StringIO()
        self._all = set()
        self._phony = set(['all', 'clean'])
        self._opt_include = set()
        self._qnames = {}
        self._qctr = 0
        self._proj = proj

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

        The target should be a string or path, and the sources should
        be a list of strings or paths.  The commands should be a list
        of commands, and each command should be a list of arguments.
        Arguments are strings, paths, or tuples consisting of strings
        and paths.  This handles escaping of all of the sources,
        targets, and commands.
        """
        convert_path = self._proj.posix
        if target.basename() == 'Makefile':
            write = self._mrulefp.write
        else:
            write = self._rulefp.write
        if isinstance(target, Path):
            write(mk_escape(convert_path(target)))
            dirs = [convert_path(target.dirname())]
        else:
            write(mk_escape(target))
            dirs = []
            self._phony.add(target)
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
                self._all.add(self._proj.posix(target))
            else:
                self._all.add(target)

    def write(self, fp):
        fp.write('all:')
        for target in sorted(self._all):
            fp.write(' ' + target)
        fp.write('\n')
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
        fp.write('.PHONY: {}'.format(' '.join(sorted(self._phony))) + '\n')
        fp.write(
            'clean:\n'
            '\trm -rf build\n'
        )
        fp.write(self._rulefp.getvalue())

    def opt_include(self, *paths):
        for path in paths:
            self._opt_include.add(self._proj.posix(path))
