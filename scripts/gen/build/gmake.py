import gen.build.graph as graph
import gen.build.target as target
import gen.atom as atom
from gen.env import Environment, VarRef, flagstr
from gen.path import Path
import gen.build.graph
import gen.build.linux as linux
import platform

def add_sources(graph, proj, env, settings):
    pass

def get_example_source(proj):
    for group in proj.sourcelist.groups():
        if group.simple_name == 'sglib':
            srcs = []
            for source in group.sources():
                if source.atoms == ('SGLIB',):
                    srcs.append(source.relpath.posix)
            srcs.sort()
            if not srcs:
                raise Exception('cannot find source')
            return srcs[0]

class AutoConf(target.Commands):
    """Linux: Run autoconf etc."""
    __slots__ = ['_env']

    def input(self):
        yield Path('configure.ac')

    def output(self):
        yield Path('configure')
        yield Path('config.h.in')

    def name(self):
        return 'AUTOCONF'

    def commands(self):
        return [['aclocal'], ['autoheader'], ['autoconf']]

class Configure(target.Commands):
    """Linux: Run configure."""
    __slots__ = ['_env', '_settings']

    def __init__(self, env, settings):
        self._env = env
        self._settings = settings

    def input(self):
        yield Path('configure')
        yield Path('config.mak.in')

    def output(self):
        yield Path('config.mak')
        yield Path('config.h')

    def commands(self):
        s = self._settings
        cflags = {
            'debug': '-O0 -g',
            'release': '-O2 -g',
        }
        try:
            cflags = cflags[s.CONFIG]
        except KeyError:
            raise ValueError('unknown configuration: %r' % (s.CONFIG,))
        return [['./configure', '--enable-warnings=error',
                 'CFLAGS=' + cflags, 'CXXFLAGS=' + cflags]]

class LookupAC(object):
    """Class for looking up autoconf template variables."""
    __slots__ = ['_proj']

    def __init__(self, proj):
        self._proj = proj

    def __call__(self, x):
        info = self._proj.info
        if info.haskey(x):
            return getattr(info, x)
        if x == 'SRCFILE':
            return get_example_source(self._proj)

def add_targets(graph, proj, env, settings):
    """Generate targets for autotools build on Linux."""
    mf = Path('Makefile')
    graph.add(Makefile(mf, proj))
    rs = Path('run.sh')
    graph.add(RunSh(rs, proj))
    cm = Path('config.mak.in')
    graph.add(ConfigMak(cm))
    ccin = Path('configure.ac')
    ccsrc = Path(proj.sgpath, 'scripts/configure.ac')
    lookup = LookupAC(proj)
    graph.add(target.Template(ccin, ccsrc, lookup, regex=r'@(\w+)@'))

    deps = [mf, cm, ccin, rs]
    if platform.system() in ('Linux', 'Darwin'):
        graph.add(AutoConf(env))
        deps.append(Path('configure'))

    deps.extend(graph.platform_built_sources(proj, 'LINUX'))
    graph.add(target.DepTarget('gmake', deps))

    if platform.system() == 'Linux':
        graph.add(Configure(env, settings))
        cdep = [Path('config.mak'), 'gmake']
        graph.add(target.DepTarget('config', cdep))
        graph.add(target.DepTarget('default', ['config']))

LIBS = ['LIBJPEG', 'GTK', 'LIBPNG', 'PANGO']
FLAGS = ['CC', 'CXX',
         'CPPFLAGS', 'CFLAGS', 'CXXFLAGS', 'CWARN', 'CXXWARN',
         'LDFLAGS', 'LIBS']

class Makefile(target.StaticFile):
    """Linux: Create a Makefile for autotools."""
    __slots__ = ['_dest', '_proj']

    def __init__(self, dest, proj):
        target.StaticFile.__init__(self, dest)
        self._proj = proj

    def write(self, f):
        proj = self._proj
        g = gen.build.graph.Graph()

        def lookup_env(atom):
            if atom not in LIBS:
                return None
            cflags = VarRef(atom + '_CFLAGS')
            return Environment(
                CFLAGS=cflags,
                CXXFLAGS=cflags,
                LIBS=VarRef(atom + '_LIBS'),
            )

        env = Environment()
        for flag in FLAGS:
            env[flag] = VarRef(flag)
        env = Environment(
            Environment(
                CFLAGS=VarRef('dep_args'),
                CXXFLAGS=VarRef('dep_args')
            ), env)

        projenv = atom.ProjectEnv(proj, lookup_env, env)

        ts = linux.genbuild_linux(g, projenv, None)
        g.add(target.DepTarget('all', ts))

        targs = g.targetobjs()
        objs = []
        prods = []
        for t in targs:
            if isinstance(t, target.CC):
                for o in t.output():
                    if isinstance(o, Path):
                        objs.append(o.posix)
            else:
                for o in t.output():
                    if isinstance(o, Path):
                        prods.append(o.posix)
        objs.sort()
        prods.sort()

        f.write(MAKE_HEAD)

        f.write('ALL_OBJS = ')
        f.write(' '.join(objs))
        f.write('\n\n')

        f.write('ALL_PRODS = ')
        f.write(' '.join(prods))
        f.write('\n\n')

        f.write(MAKE_PART2)
        g.gen_gmake(['all'], f)

class ConfigMak(target.StaticFile):
    """Linux: Create config.mak.in for autotools."""
    __slots__ = ['_dest']

    def write(self, f):
        def v(x):
            f.write('%s = @%s@\n' % (x, x))
        f.write('# Automatically generated by build system\n')
        f.write('# ===== General programs and flags =====\n')
        for flag in FLAGS:
            v(flag)
        f.write('# ===== Libraries =====\n')
        for lib in LIBS:
            v(lib + '_CFLAGS')
            v(lib + '_LIBS')

MAKE_HEAD = """\
# This file automatically generated
all:
include config.mak

ifndef V
QLD       = @echo '    LD' $@
QCC       = @echo '    CC' $@
QCXX      = @echo '    CXX' $@
QOBJCOPY  = @echo '    OBJCOPY' $@
QS        = @
endif

"""

MAKE_PART2 = """\
dep_check = $(shell \
$(CC) -c -MF /dev/null -MMD -MP -x c /dev/null -o /dev/null 2>&1; \
echo $$?)
ifeq ($(dep_check),0)
COMPUTE_DEPS = yes
else
COMPUTE_DEPS = no
endif

ifeq ($(COMPUTE_DEPS),yes)
dep_files := $(foreach f,$(ALL_OBJS),$(f).d)
dep_files_present := $(wildcard $(dep_files))
ifneq ($(dep_files_present),)
include $(dep_files_present)
endif
dep_args = -MF $@.d -MMD -MP
else
dep_args =
endif

out_dirs := $(sort $(dir $(ALL_PRODS) $(ALL_OBJS)))
dirs_missing := $(filter-out $(wildcard $(out_dirs)),$(out_dirs))
$(out_dirs):
\t@mkdir -p $@

clean:
\trm -rf build
.PHONY: clean
"""

class RunSh(target.StaticFile):
    """Linux: Create run.sh for autotools build."""
    __slots__ = ['_dest', '_proj']

    def __init__(self, dest, proj):
        target.StaticFile.__init__(self, dest)
        self._proj = proj

    def write(self, f):
        import cStringIO, re
        projenv = atom.ProjectEnv(self._proj)
        default_app = None
        for targenv in projenv.targets('LINUX'):
            if default_app is None:
                default_app = targenv.simple_name
                break
        if default_app is None:
            raise Exception('no targets')

        help = cStringIO.StringIO()
        help.write(HELP_P1)
        argp = cStringIO.StringIO()
        disp = cStringIO.StringIO()
        for targenv in projenv.targets('LINUX'):
            sname = targenv.simple_name
            desc = targenv.main.desc
            if sname == default_app:
                help.write('  %s [default]: %s\n' % (sname, desc))
            else:
                help.write('  %s: %s\n' % (sname, desc))
            path =  './build/product/%s' % (targenv.EXE_LINUX,)
            argp.write('        %s) APP=%s ; shift ;;\n' % (sname, sname))
            args = [path]
            for k, v in targenv.DEFAULT_CVARS:
                args.append('-d%s=%s' % (k, flagstr(v)))
            d = {
                'APP': sname,
                'PATH': path,
                'ARGS': ' '.join(args),
            }
            def repl(m):
                return d[m.group(1)]
            disp.write(re.sub(r'@(\w+)@', repl, DISPATCH))

        f.write(SHELL_P1.replace('@DEFAULT_APP@', default_app))
        f.write(help.getvalue())
        f.write(SHELL_P2)
        f.write(argp.getvalue())
        f.write(SHELL_P3)
        f.write(disp.getvalue())
        f.write(SHELL_P4)
        return True

    def executable(self):
        return True
            

SHELL_P1 = """\
#!/bin/sh
# This file automatically generated by build system

MODE=normal
APP=@DEFAULT_APP@

for i; do
    case "$i" in
	--gdb)
	    MODE=gdb
	    shift
	    ;;
	--valgrind)
	    MODE=valgrind
	    shift
	    ;;
        --help)
            cat >&2 <<EOF
"""

SHELL_P2 = """\
EOF
            exit 1;
            ;;
"""

SHELL_P3 = """\
	*)
	    break
	    ;;
    esac
done

check_app()
{
    if test ! -x "$1" ; then
        echo "error: $1 does not exist, did you run make?" >&2
        exit 1
    fi
}

case $APP:$MODE in
"""

SHELL_P4 = """\
esac
"""

HELP_P1 = """\
Usage: run.sh [options..] [target] [target-options..]

Run the given target.

Targets:
"""

HELP_P2 = """\

Options:
  --gdb         run the target in GDB
  --help        show this help screen
  --valgrind    run the target through Valgrind
"""

DISPATCH = """\
    @APP@:normal)
        check_app @PATH@
        exec @ARGS@ "$@"
        ;;
    @APP@:gdb)
        check_app @PATH@
        exec gdb --args @ARGS@ "$@"
        ;;
    @APP@:valgrind)
        check_app @PATH@
        exec valgrind -- @ARGS@ "$@"
        ;;
"""
