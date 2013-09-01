# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .. import nix
from build.object import GeneratedSource
from build.path import Path
from build.shell import escape
from build.error import ProjectError
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

def arg_to_string(cfg, arg):
    if isinstance(arg, str):
        return arg
    elif isinstance(arg, Path):
        return cfg.target_path(arg)
    elif isinstance(arg, tuple):
        parts = []
        for part in arg:
            if isinstance(part, str):
                parts.append(part)
            elif isinstance(part, Path):
                parts.append(cfg.target_path(part))
            else:
                raise TypeError()
        return ''.join(parts)
    else:
        raise TypeError()

class RunScript(GeneratedSource):
    __slots__ = ['exe_path', 'exe_name', 'args']
    is_regenerated_only = True

    def make_self(self, makefile, build):
        from . import gmake
        cmd = ('@if test -x {script} ; then touch {script} ; else '
               '{python} {config} regen {script} && chmod +x {script} ; fi'
               .format(script=build.cfg.target_path(self.target),
                       python=sys.executable,
                       config=sys.argv[0]))
        makefile.add_rule(self.target, [self.exe_path], [gmake.Raw(cmd)])

    def write(self, fp, cfg):
        fp.write(RUN_SCRIPT.format(
            exe=escape(cfg.target_path(self.exe_path)),
            name=escape(self.exe_name),
            args=' '.join(escape(arg_to_string(cfg, arg))
                          for arg in self.args),
        ))

class Target(nix.MakefileTarget):
    __slots__ = []

    def make_executable(self, makefile, build, module):
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
            elif src.type in ('h', 'h++'):
                pass
            else:
                build.cfg.warn(
                    'cannot add file to executable: {}'.format(src.path))

        build_env = build.get_env(self.base_env, source.deps)

        exepath = exedir.join1(filename)
        debugpath = proddir.join1(filename, '.dbg')
        prodpath = proddir.join1(filename)

        makefile.add_rule(
            exepath, objs,
            [nix.ld_cmd(build_env, exepath, objs, src_types)],
            qname='LD')
        makefile.add_rule(
            debugpath, [exepath],
            [['objcopy', '--only-keep-debug', exepath, debugpath],
             ['chmod', '-x', debugpath]],
            qname='ObjCopy')
        makefile.add_rule(
            prodpath, [exepath, debugpath],
            [['objcopy', '--strip-unneeded',
              ('--add-gnu-debuglink=', debugpath),
              exepath, prodpath]],
            qname='ObjCopy')

        scriptpath = Path('/', 'builddir').join1(filename)
        script = RunScript(
            target=scriptpath,
            exe_name=filename,
            exe_path=prodpath,
            args=module.args,
        )
        makefile.add_clean(scriptpath)
        makefile.add_default(scriptpath)
        build.add(None, script)

    def make_sources(self, makefile, build):
        objdir = Path('/build/obj', 'builddir')
        for src in build.sources.values():
            if src.type in ('c', 'c++'):
                opath = objdir.join(src.path.to_posix()).withext('.o')
                dpath = opath.withext('.d')
                build_env = build.get_env(self.base_env, src.modules,
                                          CPPPATH=src.header_paths)
                makefile.add_rule(
                    opath, [src.path],
                    [nix.cc_cmd(build_env, opath, src.path, src.type,
                                depfile=dpath, external=src.external)],
                    srctype=src.type)
                makefile.opt_include(dpath)
            elif src.type in ('h', 'h++'):
                pass
            else:
                build.cfg.warn(
                    'file has unhandled type: {}'.format(src.path))
