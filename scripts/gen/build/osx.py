import gen.build.target as target
import gen.build.nix as nix
import gen.atom as atom
from gen.env import Environment
import gen.path as path

Path = path.Path

class InfoPlist(target.StaticFile):
    """Target which creates an Info.plist file for an application."""
    __slots__ = ['_dest', '_env']
    def __init__(self, dest, env):
        target.StaticFile.__init__(dest, env)

    def write(self, f):
        try:
            icon = env['EXE_MACICON']
            icon = os.path.splitext(icon)[0]
            icon = os.path.basename(icon)
            icon = unicode(icon)
        except KeyError:
            icon = None
        try:
            copyright = env['PKG_COPYRIGHT']
            getinfo = '%s, %s' % (env['VERSION'], copyright)
        except KeyError:
            print >>sys.stderr, 'warning: PKG_COPYRIGHT is unset'
            getinfo = unicode(env['VERSION'])
        try:
            category = unicode(env['PKG_APPLE_CATEGORY'], 'ascii')
        except KeyError:
            category = u'public.app-category.games'
        plist = {
            u'CFBundleDevelopmentRegion': u'English',
            u'CFBundleExecutable': u'${EXECUTABLE_NAME}',
            u'CFBundleGetInfoString': getinfo,
            # CFBundleName
            u'CFBundleIconFile': icon,
            u'CFBundleIdentifier': unicode(env['PKG_IDENT']),
            u'CFBundleInfoDictionaryVersion': u'6.0',
            u'CFBundlePackageType': u'APPL',
            u'CFBundleShortVersionString': unicode(env['VERSION']),
            u'CFBundleSignature': u'????',
            u'CFBundleVersion': unicode(env['VERSION']),
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
        import plistxml
        f.write(plistxml.dump(plist))

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
        atoms = set([arch])
        objext = '.%s.o' % (arch,)
        objs = []
        for source in proj.sourcelist.sources():
            env = atomenv.getenv(source.atoms + (arch,))
            if env is None:
                continue
            atoms.update(source.atoms)
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

        env = atomenv.getenv(atoms - atom.PLATFORMS)
        exe = Path('build/exe', exename + '-' + arch)
        graph.add(nix.LD(exe, objs, env))
        exes.append(exe)

    env = Environment(penv, uenv)

    # combine executables into universal
    exe_raw = Path('build/exe', exename)
    graph.add(Lipo(exe_raw, exes, env))

    contents = Path('build/product', appname + '.app', 'Contents')

    # produce stripped executable and dsym package
    exe = Path(contents, 'MacOS', exename)
    dsym = Path('build/product', appname + '.app.dSYM')
    graph.add(Strip(exe, exe_raw, env))
    graph.add(DSymUtil(dsym, exe_raw, env))

    graph.add(target.DepTarget('build', [exe, dsym], env))
    return

    # Create Info.plist and PkgInfo
    pcmds = ['Set :CFBundleExecutable %s' % exename]
    # CFBundleIdentifier
    build.add(plist(os.path.join(contents, 'Info.plist'),
                    obj.info_plist(), pcmds))
    pkginfo = 'APPL????'
    build.add(target.StaticFile(os.path.join(contents, 'PkgInfo'),
                                pkginfo))

    # Compile / copy resources
    resources = os.path.join(contents, 'Resources')
    build.add(ibtool(os.path.join(resources, 'MainMenu.nib'),
                     obj.main_xib(), env))
    try:
        icon = obj.env['EXE_MACICON']
    except KeyError:
        pass
    else:
        objf = os.path.join(resources, os.path.basename(icon))
        build.add(target.CopyFile(objf, icon))
