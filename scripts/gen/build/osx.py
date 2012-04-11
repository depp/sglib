import gen.build.target as target
import gen.build.nix as nix
import gen.atom as atom
from gen.env import Environment
import gen.path as path

Path = path.Path

class PkgInfo(target.StaticFile):
    """Target which creates a PkgInfo file for an application."""
    __slots__ = ['_dest', '_env']
    def write(self, f):
        f.write('APPL????')

class InfoPlist(target.StaticFile):
    """Target which creates an Info.plist file for an application."""
    __slots__ = ['_dest', '_env']
    def write(self, f):
        env = self._env

        try:
            icon = env['EXE_MACICON']
        except KeyError:
            icon = None
        else:
            icon = unicode(icon.withext('').basename, 'ascii')

        try:
            copyright = env['PKG_COPYRIGHT']
        except KeyError:
            print >>sys.stderr, 'warning: PKG_COPYRIGHT is unset'
            getinfo = unicode(env.PKG_APP_VERSION)
        else:
            getinfo = u'%s, %s' % (env.PKG_APP_VERSION, copyright)

        try:
            category = env['PKG_APPLE_CATEGORY']
        except KeyError:
            category = u'public.app-category.games'
        else:
            category = unicode(category, 'ascii')

        plist = {
            u'CFBundleDevelopmentRegion': u'English',
            u'CFBundleExecutable': u'${EXECUTABLE_NAME}',
            u'CFBundleGetInfoString': getinfo,
            # CFBundleName
            u'CFBundleIconFile': icon,
            u'CFBundleIdentifier': unicode(env.PKG_IDENT),
            u'CFBundleInfoDictionaryVersion': u'6.0',
            u'CFBundlePackageType': u'APPL',
            u'CFBundleShortVersionString': unicode(env.PKG_APP_VERSION),
            u'CFBundleSignature': u'????',
            u'CFBundleVersion': unicode(env.PKG_APP_VERSION),
            u'LSApplicationCategory': category,
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

def add_targets(graph, proj, userenv):
    import platform
    if platform.system() == 'Darwin':
        build_osx(graph, proj, userenv)

def build_osx(graph, proj, userenv):
    """Generate targets for a normal Mac OS X build.

    This produces the actual application bundle as a target.
    """
    uenv = nix.get_user_env(userenv)
    uenv.set(
        ARCHS='ppc i386',
        CC=('gcc-4.2', 'gcc'),
        CXX=('g++-4.2', 'g++'),
    )
    uenv.override(userenv)

    libs = []
    fworks = ['Foundation', 'AppKit', 'OpenGL',
              'CoreServices', 'CoreVideo', 'Carbon']
    for fwork in fworks:
        libs.extend(('-framework', fwork))
    penv = Environment(
        proj.env,
        CFLAGS='-gdwarf-2',
        LIBS=libs,
        LDFLAGS='-Wl,-dead_strip -Wl,-dead_strip_dylibs',
    )
    env = Environment(penv, uenv)

    appname = env.EXE_MAC
    exename = appname
    archs = env.ARCHS

    archenvs = {}
    for arch in archs:
        aflags = ['-arch', arch]
        ldflags = aflags[:]
        cflags = aflags[:]
        if arch in ('ppc', 'ppc64'):
            cflags.append('-mtune=G5')
        aenv = Environment(
            LDFLAGS=ldflags,
            CFLAGS=cflags,
            CXXFLAGS=cflags,
        )
        archenvs[arch] = aenv

    def lookupenv(atom):
        try:
            return archenvs[atom]
        except KeyError:
            pass
        return None

    atomenv = atom.AtomEnv([lookupenv], penv, uenv, 'MACOSX')

    # build executable for each architecture
    exes = []
    for arch in archs:
        srcenv = atom.SourceEnv(proj, atomenv, arch)
        objext = '.%s.o' % (arch,)
        objs = []
        for source, env in srcenv:
            ext = source.grouppath.ext
            try:
                stype = path.EXTS[ext]
            except KeyError:
                raise Exception(
                    'unknown extension %s for path %s' %
                    (ext, source.relpath.posixpath))
            if stype in ('c', 'cxx', 'm', 'mm'):
                opath = Path('build/obj', source.group.name,
                             source.grouppath.withext(objext))
                objs.append(opath)
                graph.add(nix.CC(opath, source.relpath, env, stype))
            elif stype in ('h', 'hxx'):
                pass
            else:
                raise Exception(
                    'cannot handle file type %s for path %s' %
                    (stype, source.relpath.posixpath))

        env = srcenv.unionenv()
        exe = Path('build/exe', exename + '-' + arch)
        graph.add(nix.LD(exe, objs, env))
        exes.append(exe)

    env = Environment(penv, uenv)

    # combine executables into universal
    exe_raw = Path('build/exe', exename)
    graph.add(Lipo(exe_raw, exes, env))

    contents = Path('build/product', appname + '.app', 'Contents')

    appdeps = []

    # produce stripped executable and dsym package
    exe = Path(contents, 'MacOS', exename)
    dsym = Path('build/product', appname + '.app.dSYM')
    appdeps.extend((exe, dsym))
    graph.add(Strip(exe, exe_raw, env))
    graph.add(DSymUtil(dsym, exe_raw, env))

    # Create Info.plist and PkgInfo
    src_plist = Path('resources/mac/Info.plist')
    app_plist = Path(contents, 'Info.plist')
    pcmds = ['Set :CFBundleExecutable %s' % exename]
    graph.add(InfoPlist(src_plist, env))
    graph.add(Plist(app_plist, src_plist, env, pcmds))
    appdeps.append(app_plist)

    # Create PkgInfo
    app_pinfo = Path(contents, 'PkgInfo')
    graph.add(PkgInfo(app_pinfo, env))
    appdeps.append(app_pinfo)

    # Compile / copy resources
    resources = Path(contents, 'Resources')
    tpl_xib = Path(proj.sgpath, 'resources/mac/MainMenu.xib')
    src_xib = Path('resources/mac/MainMenu.xib')
    app_nib = Path(resources, 'MainMenu.nib')
    graph.add(target.Template(src_xib, tpl_xib, env))
    graph.add(IBTool(app_nib, src_xib, env))
    appdeps.append(app_nib)

    try:
        icon = env['EXE_MACICON']
    except KeyError:
        pass
    else:
        app_icon = Path(resources, icon.basename)
        graph.add(target.CopyFile(app_icon, icon, env))
        appdeps.append(app_icon)

    graph.add(target.DepTarget('build', appdeps, env))
