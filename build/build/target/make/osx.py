from .. import nix
from .. import env
from build.path import Path
from build.error import ConfigError

class Target(nix.MakefileTarget):
    __slots__ = ['archs']

    def __init__(self, subtarget, cfg, args, archs):
        super(Target, self).__init__(subtarget, cfg, args, None)
        if archs is None:
            from .. import darwin
            if cfg.config == 'debug':
                archs = darwin.default_debug_archs()
            else:
                archs = darwin.default_release_archs()
        self.archs = tuple(archs)

    def get_dirs(self, name):
        builddir = Path('/build', 'builddir')
        return {arch: builddir.join('{}-{}'.format(name, arch))
                for arch in self.archs}

    def make_application_bundle(self, makefile, build, module):
        source = build.sourcemodules[module.source]
        filename = module.filename
        archs = self.archs

        appdeps = []
        proddir = Path('/build/products', 'builddir')
        apppath = proddir.join(filename + '.app')

        build_env = env.merge_env(
            [self.base_env] +
            [build.modules[req].env for req in source.deps])

        rsrcpath = apppath.join('Contents/Resources')
        objdirs = self.get_dirs('obj')
        exedirs = self.get_dirs('exe')
        objs = {arch: [] for arch in archs}
        exes = {arch: exedir.join(filename) for arch, exedir in exedirs.items()}
        src_types = set()
        for src in source.sources:
            if src.type in ('c', 'c++', 'objc', 'objc++'):
                src_types.add(src.type)
                for arch, objdir in objdirs.items():
                    opath = objdir.join(src.path.to_posix()).withext('.o')
                    objs[arch].append(opath)
            elif src.type in ('h', 'h++'):
                pass
            elif src.type == 'xib':
                xibsrc = src.path
                nibdest = rsrcpath.join(xibsrc.basename()).withext('.nib')
                makefile.add_rule(
                    nibdest, [xibsrc],
                    [['/Developer/usr/bin/ibtool',
                      '--errors', '--warnings', '--notices',
                      '--output-format', 'human-readable-text',
                      '--compile', nibdest, xibsrc]],
                    qname='IBTool')
                appdeps.append(nibdest)
            elif src.type == 'resource':
                srcpath = src.path
                destpath = rsrcpath.join(srcpath.basename())
                makefile.add_rule(
                    destpath, [srcpath], [['cp', srcpath, destpath]],
                    qname='Copy')
                appdeps.append(destpath)
            else:
                build.cfg.warn(
                    'cannot add file to executable: {}'
                    .format(src.path))

        for arch in archs:
            arch_env = env.merge_env(
                [build_env, {'LDFLAGS': ('-arch', arch)}])
            makefile.add_rule(
                exes[arch], objs[arch],
                [nix.ld_cmd(arch_env, exes[arch], objs[arch], src_types)],
                qname='LD')

        exes = list(exes.values())
        exe1path = Path('/build/exe', 'builddir').join(filename)
        makefile.add_rule(
            exe1path, exes,
            [['lipo'] + exes + ['-create', '-output', exe1path]],
            qname='Lipo')

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

        srcplist = module.info_plist
        destplist = apppath.join('Contents/Info.plist')
        pcmds = ['Set :CFBundleExecutable {}'.format(filename)]
        pcmdline = ['/usr/libexec/PlistBuddy', '-x']
        for pcmd in pcmds:
            pcmdline.extend(('-c', pcmd))
        pcmdline.append(destplist)
        makefile.add_rule(
            destplist, [srcplist],
            [['cp', srcplist, destplist], pcmdline],
            qname='PList')
        appdeps.append(destplist)

        makefile.add_rule(
            apppath, appdeps,
            [['touch', apppath]])
        makefile.add_default(apppath)

    def make_sources(self, makefile, build):
        objdirs = self.get_dirs('obj')
        for src in build.sources.values():
            if src.type in ('c', 'c++', 'objc', 'objc++'):
                build_env = env.merge_env(
                    [self.base_env] +
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
                        srctype=src.type)
                    makefile.opt_include(dpath)
            elif src.type in ('h', 'h++', 'xib', 'resource'):
                pass
            else:
                build.cfg.warn(
                    'file has unhandled type: {}'.format(src.path))
