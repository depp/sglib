import gen.build.target as target
import gen.build.nix as nix
import gen.atom as atom
from gen.env import Environment
import gen.shell as shell
from gen.path import Path
from gen.error import *
import subprocess

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
        return 'OBJCOPY'

    def commands(self):
        return [['objcopy', '--strip-unneeded',
                 ('--add-gnu-debuglink=', self._debugsyms),
                 self._src, self._dest]]

########################################################################

def customconfig(cmd, name):
    """Get the environment for a package from a config program."""
    try:
        cflags = shell.getoutput(cmd + ['--cflags'])
        libs = shell.getoutput(cmd + ['--libs'])
    except subprocess.CalledProcessError, e:
        if e.returncode > 0:
            raise MissingPackage(name)
        raise BuildError('command failed: %s' % ' '.join(cmd))
    return Environment(CFLAGS=cflags, LIBS=libs)

def pkgconfig(pkg, name=None):
    """Get the environment for a package from pkg-config."""
    if name is None:
        name = pkg
    return customconfig(['pkg-config', pkg], name)

def add_sources(graph, proj, env, settings):
    pass

def add_targets(graph, proj, env, settings):
    import platform
    if platform.system() == 'Linux':
        build_linux(graph, proj, env, settings)

def linux_env(env, settings):
    base_env = Environment(
        CFLAGS='-g',
        CXXFLAGS='-g',
        LDFLAGS='-Wl,--as-needed -Wl,--gc-sections',
        LIBS='-lGL -lGLU',
    )
    user_env = nix.get_default_env(settings)
    user_env.update(env)
    return Environment(base_env, user_env)

def build_linux(graph, proj, env, settings):
    """Generate targets for a normal Linux build.

    This produces the actual executable as a target.
    """
    env = linux_env(env, settings)

    machine = nix.getmachine(env)
    if machine == 'x64':
        machine = 'linux64'
    elif machine == 'x86':
        machine = 'linux32'
    else:
        print >>sys.stderr, 'warning: unknown machine %s' % machine

    libs = {
        'LIBJPEG':    lambda: Environment(LIBS='-ljpeg'),
        'GTK':        lambda: pkgconfig('gtk+-2.0 gtkglext-1.0', 'gtkglext'),
        'LIBPNG':     lambda: pkgconfig('libpng'),
        'PANGO':      lambda: pkgconfig('pangocairo'),
        'LIBASOUND':  lambda: pkgconfig('alsa'),
    }

    def lookup_env(atom):
        try:
            thunk = libs[atom]
        except KeyError:
            return None
        else:
            env = thunk()
            # This flag is added by the gmodule indirect dependency.
            # We don't need this flag and it's a waste.
            env.remove_flag('LIBS', '-Wl,--export-dynamic')
            return env

    projenv = atom.ProjectEnv(proj, lookup_env, env)
    products = genbuild_linux(graph, projenv, machine)
    graph.add(target.DepTarget('build', products))

def genbuild_linux(graph, projenv, machine):
    """Generate all targets for any Linux build.

    The machine parameter is the machine name to add to the name of
    the executable.  It may be empty.
    """

    apps = []
    types_cc = 'c', 'cxx'
    types_ignore = 'h', 'hxx'
    for targetenv in projenv.targets('LINUX'):
        mname = targetenv.simple_name
        objs = []
        def handlec(source, env):
            opath = Path('build/obj-%s-%s' %
                         (mname, source.group.simple_name),
                         source.grouppath.withext('.o'))
            objs.append(opath)
            graph.add(nix.CC(opath, source.relpath, env, source.sourcetype))
        handlers = {}
        for t in types_cc: handlers[t] = handlec
        for t in types_ignore: handlers[t] = None
        targetenv.apply(handlers)

        env = targetenv.unionenv()
        exename = targetenv.EXE_LINUX
        if machine:
            exename = '%s-%s' % (exename, machine)
        rawpath = Path('build/exe-%s' % (mname,), exename)
        exepath = Path('build/product', exename)
        dbgpath = Path('build/product', exename + '.dbg')

        graph.add(nix.LD(rawpath, objs, env, targetenv.types()))
        graph.add(ExtractDebug(dbgpath, rawpath, env))
        graph.add(Strip(exepath, rawpath, dbgpath, env))

        pseudo = 'build-%s' % (mname,)
        graph.add(target.DepTarget(pseudo, [exepath, rawpath]))
        apps.append(pseudo)

    return apps
