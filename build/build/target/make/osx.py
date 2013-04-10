from .. import nix
from .. import env
from . import gmake
from build.path import Path
import sys

class Target(gmake.Target):
    __slots__ = ['archs']

    def get_dirs(self, name):
        builddir = Path('/build', 'builddir')
        return {arch: builddir.join('{}-{}'.format(name, arch))
                for arch in self.archs}

    def __init__(self, subtarget, osname, args):
        super(Target, self).__init__(subtarget, osname, args)
        self.archs = ['ppc', 'i386', 'x86_64']

    def build_application_bundle(self, makefile, build, module):
        source = build.modules[module.source]
        filename = module.filename

        build_env = env.merge_env(
            [self.env] +
            [build.modules[req].env for req in source.deps])

        objdirs = self.get_dirs('obj')
        exedirs = self.get_dirs('exe')
        exes = []
        for arch in self.archs:
            objdir = objdirs[arch]
            exedir = exedirs[arch]
            objs = []
            src_types = set()
            for src in source.sources:
                if src.type in ('c', 'c++', 'objc', 'objc++'):
                    src_types.add(src.type)
                    opath = objdir.join(src.path.to_posix()).withext('.o')
                    objs.append(opath)
                elif src.type in ('header',):
                    pass
                else:
                    print('warning: ignored source file: {}'
                          .format(src.path), file=sys.stderr)

            arch_env = env.merge_env(
                [build_env, {'LDFLAGS': ('-arch', arch)}])
            exepath = exedir.join(filename)

            makefile.add_rule(
                exepath, objs,
                [nix.ld_cmd(arch_env, exepath, objs, src_types)],
                'LD')
            exes.append(exepath)

        exe1path = Path('/build/exe', 'builddir').join(filename)
        makefile.add_rule(
            exe1path, exes,
            [['lipo'] + exes + ['-create', '-output', exe1path]],
            qname='Lipo')

        appdeps = []
        proddir = Path('/build/products', 'builddir')

        apppath = proddir.join(filename + '.app')
        exe2path = apppath.join('Contents/MacOS', filename)
        makefile.add_rule(
            exe2path, [exe1path],
            [['strip', '-u', '-r', '-o', exe2path, exe1path]],
            qname='Strip')
        appdeps.append(exe2path)

        debugpath = proddir.join(filename + '.app.dSYM')
        makefile.add_rule(
            debugpath, [exe1path],
            [['dsymutil', exe1path, '-o', debugpath]],
            qname='DSym')
        appdeps.append(debugpath)

        makefile.add_rule(
            apppath, appdeps,
            [['touch', apppath]])
        makefile.add_default(apppath)

    def build_sources(self, makefile, build):
        objdirs = self.get_dirs('obj')
        for src in build.sources.values():
            if src.type in ('c', 'c++', 'objc', 'objc++'):
                build_env = env.merge_env(
                    [self.env] +
                    [build.modules[req].env for req in src.modules] +
                    [{'CPPPATH': src.header_paths}])
                for arch, objdir in objdirs.items():
                    opath = objdir.join(src.path.to_posix()).withext('.o')
                    dpath = opath.withext('.d')
                    flags = '-arch', arch
                    arch_env = env.merge_env([
                        build_env, {'CFLAGS':flags, 'CXXFLAGS':flags}])
                    makefile.add_rule(
                        opath, [src.path],
                        [nix.cc_cmd(arch_env, opath, src.path, src.type,
                                    depfile=dpath, external=src.external)],
                        gmake.BUILD_NAMES.get(src.type))
                    makefile.opt_include(dpath)
