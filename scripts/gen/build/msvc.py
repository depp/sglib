import gen.path
import gen.build.target as target
from gen.env import Environment
import gen.atom as atom
import subprocess
import os
import sys
import platform

Path = gen.path.Path

def getreg(key, subkey):
    """Get a value from the Windows registry."""
    import _winreg
    h = None
    try:
        h = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, key)
        i = 0
        while True:
            name, value, type = _winreg.EnumValue(h, i)
            if name == subkey:
                return value
            i += 1
    except WindowsError:
        return None
    finally:
        if h is not None:
            _winreg.CloseKey(h)

def msvc_fail(why):
    print >>sys.stderr, 'Cannot find MSVC environment: %s' % why
    sys.exit(1)

def get_sdk_environ(target, config):
    """Return the environment variables for the Windows SDK."""
    dumpenv = False
    if dumpenv:
        print '==== Initial environment ===='
        for k, v in sorted(os.environ.items()):
            print k, '=', v
        print
    p = getreg(r'SOFTWARE\Microsoft\Microsoft SDKs\Windows',
               'CurrentInstallFolder')
    if p is None:
        msvc_fail('Windows SDK is not installed')
    p = os.path.join(p, 'Bin', 'SetEnv.Cmd')
    if target not in ('x86', 'x64', 'ia64'):
        msvc_fail('target must be x86, x64, or ia64')
    if config not in ('debug', 'release'):
        msvc_fail('config must be debug or release')
    args = ' '.join(['/' + config, '/' + target, '/xp'])
    tag = 'TAG8306'
    # It seems that setenv gives an exit status of 1 normally
    cmd = 'cmd.exe /c "%s" %s & echo %s && set' % (p, args, tag)
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if proc.returncode:
        sys.stderr.write(out)
        sys.stderr.write(err)
        msvc_fail('vcvarsall.bat failed (return code=%d)' % proc.returncode)
    lines = iter(out.splitlines())
    for line in lines:
        line = line.rstrip()
        if dumpenv:
            print line
        if line == tag:
            break
    else:
        msvc_fail('vcvarsall.bat failed (formatting)')
    env = {}
    for line in lines:
        k, v = line.rstrip().split('=', 1)
        env[k] = v
    if dumpenv:
        print '==== MSVC Environment ===='
        for k, v in sorted(env.items()):
            print k, '=', v
        print
    return env

class CC(target.CC):
    """Windows: Compile a C or C++ file."""
    __slots__ = ['_dest', '_src', '_env', '_sourcetype']
    def commands(self):
        env = self._env
        if self._sourcetype == 'c':
            cc = env.CC
            cflags = env.CFLAGS
            warn = env.CWARN
            lflag = '/TC'
        elif self._sourcetype == 'cxx':
            cc = env.CXX
            cflags = env.CXXFLAGS
            warn = env.CXXWARN
            lflag = '/TP'
        else:
            assert False
        return [[cc, '/c', ('/Fo', self._dest), lflag] +
                [('/I', p) for p in env.CPPPATH] +
                env.CPPFLAGS + warn + cflags + [self._src]]

class LD(target.LD):
    """Windows: Link an executable."""
    __slots__ = ['_dest', '_src', '_env', '_debugsym']
    def __init__(self, dest, src, env, debugsym):
        target.LD.__init__(self, dest, src, env)
        if debugsym is not None and not isinstance(debugsym, Path):
            raise TypeError('debugsym must be None or Path')
        self._debugsym = debugsym

    def output(self):
        yield self._dest
        if self._debugsym is not None:
            yield self._debugsym

    def commands(self):
        env = self._env
        if self._debugsym is not None:
            ds = [('/PDB:', self._debugsym)]
        else:
            ds = []
        return [[env.LD, ('/OUT:', self._dest)] + env.LDFLAGS + ds +
                self._src + env.LIBS]

def add_sources(graph, proj, userenv):
    pass

def add_targets(graph, proj, userenv):
    if platform.system() == 'Windows':
        build_windows(graph, proj, userenv)

def build_windows(graph, proj, userenv):
    configs = {
        'debug':   ('/Od /Oy-', '/MDd'),
        'release': ('/O2 /Oy-', '/MT'),
    }
    try:
        oflags, rtflags = configs[userenv.CONFIG]
    except KeyError:
        raise Exception('unsupported configuration: %r' % \
            (env.CONFIG,))

    # Get the user environment
    ccwarn = '/W3 /WX-'
    uenv = Environment(
        ARCHS='x86 x64',
        CC='cl.exe',
        CXX='cl.exe',
        CPPFLAGS='',
        CFLAGS=oflags,
        CXXFLAGS=oflags,
        CWARN=ccwarn,
        CXXWARN=ccwarn,
        LD='link.exe',
        LDFLAGS='',
        LIBS='',
    )
    uenv.override(userenv)

    # Get the base environment
    defs = ['_CRT_SECURE_NO_DEPRECATE', 'WIN32', 'DEBUG', '_WINDOWS']
    cppflags = ['/D' + deff for deff in defs]
    ccflags = [
        '/nologo',      # don't print version info
        '/EHsc',        # assume extern C never throws exceptions
        #'/RTC1',        # run-time error checking
        '/GS',          # buffer overrun check
        '/fp:precise',  # precise floating point semantics
        '/Zc:wchar_t',  # wchar_t extension
        '/Zc:forScope', # standard for scope rules
        '/Gd',          # use __cdecl calling convention
        '/Zi',          # generate debugging info
    ]
    ldflags = [
        '/NOLOGO',      # don't print version info
        '/OPT:REF',     # dead code elimination
        '/OPT:ICF',     # identical constant fol/folding
        '/DYNAMICBASE', # allow ASLR
        '/NXCOMPAT',    # allow NX bit
        '/DEBUG',
    ]
    libs = [
        'kernel32.lib',
        'user32.lib',
        'gdi32.lib',
        'winspool.lib',
        'comdlg32.lib',
        'advapi32.lib',
        'shell32.lib',
        'ole32.lib',
        'oleaut32.lib',
        'uuid.lib',
        'odbc32.lib',
        'odbccp32.lib',
    ]
    penv = Environment(
        proj.env,
        CPPFLAGS=cppflags,
        CFLAGS=ccflags,
        CXXFLAGS=ccflags,
        LDFLAGS=ldflags,
        LIBS=libs,
    )

    env = Environment(penv, uenv)
    appname = env.EXE_WINDOWS
    archs = env.ARCHS

    # Get the environment for each architecture
    archenvs = {}
    for arch in archs:
        objdir = Path('build/obj-' + arch)
        cflags = [('/Fd', Path(objdir, 'debug.pdb'))]
        aenv = Environment(
            LDFLAGS=['/MACHINE:' + arch],
            CFLAGS=cflags,
            CXXFLAGS=cflags,
        )
        aenv.environ.update(get_sdk_environ(arch, env.CONFIG))
        archenvs[arch] = aenv

    def lookupenv(atom):
        try:
            return archenvs[atom]
        except KeyError:
            return None

    atomenv = atom.AtomEnv([lookupenv], penv, uenv, 'WINDOWS')

    # Build the executable for each architecture
    exes = []
    types_cc = 'c', 'cxx'
    types_ignore = 'h', 'hxx'
    for arch in archs:
        srcenv = atom.SourceEnv(proj, atomenv, arch)
        objdir = Path('build/obj-' + arch)
        if arch == 'x86':
            exename = appname
        else:
            exename = exename + '-' + arch

        # Build the sources
        objs = []
        def handlec(source, env):
            opath = Path(objdir, source.group.simple_name,
                         source.grouppath.withext('.obj'))
            objs.append(opath)
            graph.add(CC(opath, source.relpath, env, source.sourcetype))
        handlers = {}
        for t in types_cc: handlers[t] = handlec
        for t in types_ignore: handlers[t] = None
        srcenv.apply(handlers)

        env = Environment(
            srcenv.unionenv(),
            LDFLAGS=['/SUBSYSTEM:WINDOWS'],
        )
        pdbpath = Path('build/product', exename + '.pdb')
        exepath = Path('build/product', exename + '.exe')
        graph.add(LD(exepath, objs, env, pdbpath))
        exes.append(exepath)
        exes.append(pdbpath)

    env = Environment(penv, uenv)
    graph.add(target.DepTarget('build', exes, env))
