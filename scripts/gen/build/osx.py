from __future__ import with_statement
import gen.build.target as target
import gen.build.nix as nix
import gen.atom as atom
from gen.env import Environment
import gen.path as path
import sys
import platform
import os

Path = path.Path

class PkgInfo(target.StaticFile):
    """Target which creates a PkgInfo file for an application."""
    __slots__ = ['_dest']

    def write(self, f):
        f.write('APPL????')

class InfoPlist(target.StaticFile):
    """Target which creates an Info.plist file for an application."""
    __slots__ = ['_dest', '_pinfo', '_tinfo']
    def __init__(self, dest, pinfo, tinfo):
        target.StaticFile.__init__(self, dest)
        self._pinfo = pinfo
        self._tinfo = tinfo

    def write(self, f):
        pinfo = self._pinfo
        tinfo = self._tinfo

        try:
            icon = tinfo.EXE_MACICON
        except AttributeError:
            icon = None
        else:
            icon = unicode(icon, 'ascii')

        try:
            copyright = pinfo.PKG_COPYRIGHT
        except AttributeError:
            print >>sys.stderr, 'warning: PKG_COPYRIGHT is unset'
            getinfo = unicode(pinfo.PKG_APP_VERSION)
        else:
            getinfo = u'%s, %s' % (pinfo.PKG_APP_VERSION, copyright)

        try:
            category = tinfo.EXE_APPLE_CATEGORY
        except AttributeError:
            category = u'public.app-category.games'
        else:
            category = unicode(category, 'ascii')

        plist = {
            u'CFBundleDevelopmentRegion': u'English',
            u'CFBundleExecutable': u'${EXECUTABLE_NAME}',
            u'CFBundleGetInfoString': getinfo,
            # CFBundleName
            u'CFBundleIconFile': icon,
            u'CFBundleIdentifier': unicode(pinfo.PKG_IDENT, 'ascii'),
            u'CFBundleInfoDictionaryVersion': u'6.0',
            u'CFBundlePackageType': u'APPL',
            u'CFBundleShortVersionString':
                unicode(pinfo.PKG_APP_VERSION, 'ascii'),
            u'CFBundleSignature': u'????',
            u'CFBundleVersion':
                unicode(pinfo.PKG_APP_VERSION, 'ascii'),
            u'LSApplicationCategoryType': category,
            # LSArchicecturePriority
            # LSFileQuarantineEnabled
            u'LSMinimumSystemVersion': u'10.5.0',
            u'NSMainNibFile': u'MainMenu',
            u'NSPrincipalClass': u'GApplication',
            # NSSupportsAutomaticTermination
            # NSSupportsSuddenTermination
        }
        plist = dict((k,v) for (k,v) in plist.iteritems() if v is not None)
        import gen.plistxml
        f.write(gen.plistxml.dump(plist))

class Strip(target.Commands):
    """Mac OS X: Strip an executable of its debugging symbols."""
    __slots__ = ['_dest', '_src', '_env']

    def __init__(self, dest, src, env):
        target.Commands.__init__(self, env)
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        if not isinstance(src, Path):
            raise TypeError('src must be Path')
        self._dest = dest
        self._src = src

    def input(self):
        yield self._src

    def output(self):
        yield self._dest

    def name(self):
        return 'STRIP'

    def commands(self):
        env = self._env
        return [['strip', '-u', '-r', '-o', self._dest, self._src]]

class DSymUtil(target.Commands):
    """Mac OS X: Copy debugging symbols to a bundle."""
    __slots__ = ['_dest', '_src', '_env']

    def __init__(self, dest, src, env):
        target.Commands.__init__(self, env)
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        if not isinstance(src, Path):
            raise TypeError('src must be Path')
        self._dest = dest
        self._src = src

    def input(self):
        yield self._src

    def output(self):
        yield self._dest

    def name(self):
        return 'DSYM'

    def commands(self):
        env = self._env
        return [['dsymutil', self._src, '-o', self._dest]]

class Lipo(target.Commands):
    """Mac OS X: Create a universal executable from non-universal ones."""
    __slots__ = ['_dest', '_src', '_env']

    def __init__(self, dest, src, env):
        target.Commands.__init__(self, env)
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        src = list(src)
        for s in src:
            if not isinstance(s, Path):
                raise TypeError('src must be iterable of Path objects')
        self._dest = dest
        self._src = src

    def input(self):
        return iter(self._src)

    def output(self):
        yield self._dest

    def commands(self):
        return [['lipo'] + self._src + ['-create', '-output', self._dest]]

    def name(self):
        return 'LIPO'

class IBTool(target.Commands):
    """Mac OS X: Create a NIB file from a XIB file."""
    __slots__ = ['_dest', '_src', '_env']

    def __init__(self, dest, src, env):
        target.Commands.__init__(self, env)
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        if not isinstance(src, Path):
            raise TypeError('src must be Path')
        self._dest = dest
        self._src = src

    def input(self):
        yield self._src

    def output(self):
        yield self._dest

    def commands(self):
        env = self._env
        ibtool = '/Developer/usr/bin/ibtool'
        return [[ibtool, '--errors', '--warnings', '--notices',
                 '--output-format', 'human-readable-text',
                 '--compile', self._dest, self._src]]

    def name(self):
        return 'IBTOOL'

class Plist(target.Commands):
    """Mac OS X: Update a property list."""
    __slots__ = ['_dest', '_src', '_env', '_changes']

    def __init__(self, dest, src, env, changes):
        target.Commands.__init__(self, env)
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        if not isinstance(src, Path):
            raise TypeError('src must be Path')
        self._dest = dest
        self._src = src
        self._changes = changes

    def input(self):
        yield self._src

    def output(self):
        yield self._dest

    def commands(self):
        env = self._env
        buddy = '/usr/libexec/PlistBuddy'
        cmd = [buddy, '-x']
        for change in self._changes:
            cmd.extend(('-c', change))
        cmd.append(self._dest)
        return [['cp', self._src, self._dest], cmd]

    def name(self):
        return 'PLIST'

class XcodeProject(target.Target):
    """Mac OS X: Create an Xcode project."""
    __slots__ = ['_dest', '_proj']

    def __init__(self, dest, proj):
        self._dest = dest
        self._proj = proj

    def input(self):
        return iter(())

    def output(self):
        yield self._dest

    def build(self, verbose):
        print 'XCODE', self._dest.posix
        d = self._dest
        import shutil
        path = d.native
        if os.path.exists(path):
            shutil.rmtree(path)
        os.mkdir(path)
        pp = os.path.join(path, 'project.pbxproj')
        up = os.path.join(path, 'default.pbxuser')
        import gen.xcode.create
        with open(pp, 'wb') as pf:
            with open(up, 'wb') as uf:
                gen.xcode.create.write_project(self._proj, pf, uf)
        return True

########################################################################

class TargetLookup(object):
    __slots__ = ['_pinfo', '_tinfo']

    def __init__(self, pinfo, tinfo):
        self._pinfo = pinfo
        self._tinfo = tinfo

    def __call__(self, key):
        for d in (self._pinfo, self._tinfo):
            if d.haskey(key):
                return getattr(d, key)

def add_sources(graph, proj, env, settings):
    rsrc = proj.sourcelist.get_group('Resources', Path('resources'))
    atoms = ('MACOSX',)

    for module in proj.targets():
        atoms = 'MACOSX', module.atom
        rpath = Path('mac', module.atom.lower())

        src = rsrc.add(Path(rpath, 'Info.plist'), atoms)
        graph.add(InfoPlist(src.relpath, proj.info, module.info))

        src = rsrc.add(Path(rpath, 'MainMenu.xib'), atoms)
        lookup = TargetLookup(proj.info, module.info)
        graph.add(target.Template(
            src.relpath,
            Path(proj.sgpath, 'resources/mac/MainMenu.xib'),
            lookup))

def add_targets(graph, proj, env, settings):
    if platform.system() == 'Darwin':
        build_osx(graph, proj, env, settings)
    build_xcodeproj(graph, proj, env, settings)

def build_xcodeproj(graph, proj, env, settings):
    xp = Path(proj.info.PKG_FILENAME + '.xcodeproj')
    graph.add(XcodeProject(xp, proj))
    deps = [xp]
    deps.extend(graph.platform_built_sources(proj, 'MACOSX'))
    graph.add(target.DepTarget('xcode', deps))
    if platform.system() == 'Darwin':
        graph.add(target.DepTarget('default', ['xcode']))

def osx_env(env, settings):
    libs = []
    fworks = ['Foundation', 'AppKit', 'OpenGL',
              'CoreServices', 'CoreVideo', 'Carbon']
    for fwork in fworks:
        libs.extend(('-framework', fwork))
    base_env = Environment(
        CFLAGS='-gdwarf-2',
        LIBS=libs,
        LDFLAGS='-Wl,-dead_strip -Wl,-dead_strip_dylibs',
    )
    user_env = nix.get_default_env(settings)
    user_env.set(
        CC=('gcc-4.2', 'gcc'),
        CXX=('g++-4.2', 'g++'),
    )
    user_env.update(env)
    env = Environment(base_env, user_env)
    if 'ARCHS' not in user_env:
        if settings.CONFIG == 'release':
            env.ARCHS = 'ppc x86'
        elif settings.CONFIG == 'debug':
            env.ARCHS = [nix.getmachine(env)]
    return env

# Map from the architecture flags we use to the flags Apple's GCC uses
ARCH_MAP = { 'x86': 'i386', 'x64': 'x86_64' }

def build_osx(graph, proj, env, settings):
    """Generate targets for a normal Mac OS X build.

    This produces the actual application bundle as a target.
    """
    env = osx_env(env, settings)

    def lookup_env(atom):
        if atom in env.ARCHS:
            arch = ARCH_MAP.get(atom, atom)
            aflags = ['-arch', arch]
            ldflags = aflags[:]
            cflags = aflags[:]
            if arch in ('ppc', 'ppc64'):
                cflags.append('-mtune=G5')
            return Environment(
                LDFLAGS=ldflags,
                CFLAGS=cflags,
                CXXFLAGS=cflags,
            )
        return None

    projenv = atom.ProjectEnv(proj, lookup_env, env)

    apps = []
    types_cc = 'c', 'cxx', 'm', 'mm'
    types_rsrc = 'h', 'hxx', 'plist', 'xib', 'icns'
    for targenv in projenv.targets('MACOSX'):
        mname = targenv.simple_name
        appname = targenv.EXE_MAC
        exename = appname

        # build executable for each architecture
        exes = []
        for arch in env.ARCHS:
            targenv.arch = arch
            objs = []
            def handlec(source, env):
                opath = Path(
                    'build/obj-%s-%s-%s' %
                    (mname, arch, source.group.simple_name),
                    source.grouppath.withext('.o'))
                objs.append(opath)
                graph.add(nix.CC(opath, source.relpath,
                                 env, source.sourcetype))
            handlers = {}
            for t in types_cc: handlers[t] = handlec
            for t in types_rsrc: handlers[t] = None
            targenv.apply(handlers)

            uenv = targenv.unionenv()
            exe = Path('build/exe-%s' % (mname,),
                       '%s-%s' % (exename, arch))
            graph.add(nix.LD(exe, objs, uenv, targenv.types()))
            exes.append(exe)
        targenv.arch = None

        appdeps = []
        contents = Path('build/product', appname + '.app', 'Contents')
        resources = Path(contents, 'Resources')

        # combine executables into universal
        exe_raw = Path('build/exe', exename)
        graph.add(Lipo(exe_raw, exes, env))

        # produce stripped executable and dsym package
        exe = Path(contents, 'MacOS', exename)
        dsym = Path('build/product', appname + '.app.dSYM')
        appdeps.extend((exe, dsym))
        graph.add(Strip(exe, exe_raw, env))
        graph.add(DSymUtil(dsym, exe_raw, env))

        # Create Info.plist
        src_plist = Path('resources/mac/%s/Info.plist' % (mname,))
        app_plist = Path(contents, 'Info.plist')
        pcmds = ['Set :CFBundleExecutable %s' % exename]
        graph.add(Plist(app_plist, src_plist, env, pcmds))
        appdeps.append(app_plist)

        # Create PkgInfo
        app_pinfo = Path(contents, 'PkgInfo')
        graph.add(PkgInfo(app_pinfo))
        appdeps.append(app_pinfo)

        # handle resources
        handlers = {}
        for t in types_cc + types_rsrc:
            handlers[t] = None
        def handle_icns(source, env):
            targ = Path(resources, source.grouppath.basename)
            graph.add(targenv.CopyFile(targ, source.relpath, env))
            appdeps.append(targ)
        def handle_xib(source, env):
            targ = Path(resources, source.grouppath.withext('.nib').basename)
            graph.add(IBTool(targ, source.relpath, env))
            appdeps.append(targ)
        handlers['icns'] = handle_icns
        handlers['xib'] = handle_xib
        targenv.apply(handlers)

        pseudo = 'build-%s' % (mname,)
        graph.add(target.DepTarget(pseudo, appdeps))
        apps.append(pseudo)

    graph.add(target.DepTarget('build', apps))