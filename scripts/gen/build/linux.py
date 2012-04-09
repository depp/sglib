import gen.build.target as target
import gen.build.nix as nix
from gen.env import Environment
import gen.shell as shell
import gen.path as path

Path = path.Path

class ExtractDebug(target.Commands):
    """Extract debug symbols from an executable."""
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
        return 'OBJCOPY'

    def commands(self):
        return [['objcopy', '--only-keep-debug', self._src, self._dest],
                ['chmod', '-x', self._dest]]

class Strip(target.Commands):
    """Copy an executable, stripping the debug symbols from it.

    Add a link to the external debug symbols.
    """
    __slots__ = ['_dest', '_src', '_debugsyms', '_env']
    def __init__(self, dest, src, debugsyms, env):
        target.Commands.__init__(self, env)
        if not isinstance(dest, Path):
            raise TypeError('dest must be Path')
        if not isinstance(src, Path):
            raise TypeError('src must be Path')
        if not isinstance(debugsyms, Path):
            raise TypeError('debugsymse must be Path')
        self._dest = dest
        self._src = src
        self._debugsyms = debugsyms

    def input(self):
        yield self._src
        yield self._debugsyms

    def output(self):
        yield self._dest

    def name(self):
        yield 'OBJCOPY'

    def commands(self):
        return [['objcopy', '--strip-unneeded',
                 ('--add-gnu-debuglink=', self._debugsyms),
                 self._src, self._dest]]

########################################################################

def customconfig(cmd):
    """Get the environment for a package from a config program."""
    cflags = shell.getoutput(cmd + ['--cflags'])
    libs = shell.getoutput(cmd + ['--libs'])
    return Environment(CFLAGS=cflags, LIBS=libs)

def pkgconfig(pkg):
    """Get the environment for a package from pkg-config."""
    return customconfig(['pkg-config', pkg])

def getmachine(env):
    """Get the name of the target machine."""
    m = shell.getoutput([env.CC, '-dumpmachine'] +
                        env.CPPFLAGS + env.CFLAGS)
    i = m.find('-')
    if i < 0:
        raise Exception('unable to parse machine name: %r' % (m,))
    return m[:i]

def add_targets(graph, proj, userenv):
    import platform
    if platform.system() == 'Linux':
        build_linux(graph, proj, userenv)

def build_linux(graph, proj, userenv):
    """Generate targets for a normal Linux build.

    This produces the actual executable as a target.
    """
    penv = Environment(
        proj.env,
        CFLAGS='-g',
        CXXFLAGS='-g',
        LDFLAGS='-Wl,--as-needed -Wl,--gc-sections',
        LIBS='-lGL -lGLU',
    )
    uenv = nix.get_user_env(userenv)

    machine = getmachine(uenv)
    if machine == 'x86_64':
        machine = 'linux64'
    elif re.match(r'i\d86', machine):
        machine = 'linux32'
    else:
        print >>sys.stderr, 'warning: unknown machine %s' % machine

    libs = {
        'LIBJPEG':  lambda: Environment(LIBS='-ljpeg'),
        'GTK':      lambda: pkgconfig('gtk+-2.0 gtkglext-1.0'),
        'LIBPNG':   lambda: pkgconfig('libpng'),
        'PANGO':    lambda: pkgconfig('pangocairo'),
    }

    def libenvf(atom):
        try:
            thunk = libs[atom]
        except KeyError:
            return None
        else:
            env = thunk()
            # This flag is added by the gmodule indirect dependency.
            # We don't need this flag and it's a waste.
            env.remove('LIBS', '-Wl,--export-dynamic')
            return env

    products = genbuild_linux(graph, proj, penv, uenv, libenvf, machine)
    graph.add(target.DepTarget('build', products, uenv))

def genbuild_linux(graph, proj, pkgenv, userenv, libenvf, machine):
    """Generate all targets for any Linux build.

    The package environment is pkgenv, the user environment is
    userenv.

    The libenvf parameter is a function which looks up the environment
    for the library identified by the given atom.  If it returns None
    for any atom, then sources with that atom will be excluded from
    the build.

    The machine parameter is the machine name to add to the name of
    the executable.  It may be empty.
    """

    envs1 = {}
    def getenv1(atom):
        """Memoized version of libenvf."""
        try:
            return envs1[atom]
        except KeyError:
            e = libenvf(atom)
            envs1[atom] = e
            return e

    envs = {}
    allatoms = set()
    def getenv(atoms):
        """Multiple-atom version of libenvf."""
        x = list(sorted(set(atoms)))
        xn = ' '.join(x)
        try:
            return envs[xn]
        except KeyError:
            es = [pkgenv]
            for a in x:
                e = getenv1(a)
                if e is None:
                    envs[xn] = None
                    return None
                es.append(e)
            es.append(userenv)
            e = Environment(*es)
            envs[xn] = e
            allatoms.update(x)
            return e

    objs = []
    for source in proj.sourcelist.sources():
        if not source.match_platform('LINUX'):
            continue
        env = getenv(source.other_atoms())
        if env is None:
            continue
        ext = source.grouppath.ext
        try:
            stype = path.EXTS[ext]
        except KeyError:
            raise Exception(
                'unknown extension %s for path %s' %
                (ext, source.relpath.posixpath))
        if stype in ('c', 'cxx'):
            opath = Path('build/obj', source.group.name,
                         source.grouppath.withext('.o'))
            objs.append(opath)
            graph.add(nix.CC(opath, source.relpath, env, stype))
        elif stype in ('h', 'hxx'):
            pass
        else:
            raise Exception(
                'cannot handle file type %s for path %s' %
                (stype, source.relpath.posixpath))

    env = getenv(allatoms)
    if machine:
        exename = '%s-%s' % (env.EXE_LINUX, machine)
    else:
        exename = env.EXE_LINUX
    rawpath = Path('build/exe', exename)
    exepath = Path('build/product', exename)
    dbgpath = Path('build/product', exename + '.dbg')

    graph.add(nix.LD(rawpath, objs, env))
    graph.add(ExtractDebug(dbgpath, rawpath, env))
    graph.add(Strip(exepath, rawpath, dbgpath, env))

    return [exepath, rawpath]
