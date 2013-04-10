from .. import nix
from .. import env
from .. import gensource
from .. import ExeModule
from . import gmake
from build.shell import escape
from build.path import Path
import sys

RUN_SCRIPT = '''\
#!/bin/sh
exe={exe}
name={name}
if test ! -x "$exe" ; then
  cat 1>&2 <<EOF
error: $exe does not exist
Did you remember to run 'make'?
EOF
  exit 1
fi
case "$1" in
  --gdb)
    shift
    exec gdb --args "$exe" {args} "$@"
    ;;
  --valgrind)
    shift
    exec valgrind -- "$exe" {args} "$@"
    ;;
  --help)
    cat <<EOF
Usage: $name [--help | --gdb | --valgrind] [options...]
EOF
    exit 0
    ;;
  *)
    exec "$exe" {args} "$@"
    ;;
esac
'''

def arg_to_string(proj, arg):
    if isinstance(arg, str):
        return arg
    elif isinstance(arg, Path):
        return proj.posix(arg)
    elif isinstance(arg, tuple):
        parts = []
        for part in arg:
            if isinstance(part, str):
                parts.append(part)
            elif isinstance(part, Path):
                parts.append(proj.posix(part))
            else:
                raise TypeError()
        return ''.join(parts)
    else:
        raise TypeError()

class RunScript(gensource.GeneratedSource):
    __slots__ = ['exe_path', 'exe_name', 'args']
    is_regenerated_only = True

    def __init__(self, name, target, exe_path, exe_name, args):
        super(RunScript, self).__init__(name, target)
        self.exe_path = exe_path
        self.exe_name = exe_name
        self.args = args

    def write(self, fp):
        fp.write(RUN_SCRIPT.format(
            exe=escape(self.exe_path),
            name=escape(self.exe_name),
            args=' '.join(escape(arg) for arg in self.args),
        ))

class Target(gmake.Target):
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

        scriptpath = Path('/', 'builddir').join(filename)
        script = RunScript(
            None, scriptpath, build.proj.posix(exepath), filename,
            [arg_to_string(build.proj, arg) for arg in module.args])
        build.add(script)
        cmd = ('@if test -x {script} ; then touch {script} ; else '
               '{python} {config} regen {script} && chmod +x {script} ; fi'
               .format(script=build.proj.posix(scriptpath),
                       python=sys.executable,
                       config=sys.argv[0]))
        makefile.add_rule(scriptpath, [prodpath], [gmake.Raw(cmd)])
        makefile.add_clean(scriptpath)
        makefile.add_default(scriptpath)

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
                    gmake.BUILD_NAMES.get(src.type))
                makefile.opt_include(dpath)
