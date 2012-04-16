import gen.path
import gen.build.target as target
from gen.env import Environment
import gen.atom as atom
import gen.version as version
import gen.msvc as msvc
import subprocess
import os
import sys
import platform
import re

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

_SDK_LOC = None
def get_sdk_loc():
    """Get the location of the latest Windows SDK."""
    global _SDK_LOC
    if _SDK_LOC is not None:
        return _SDK_LOC
    is_version = re.compile(r'^v\d')
    import _winreg
    h = None
    try:
        key = r'SOFTWARE\Microsoft\Microsoft SDKs\Windows'
        h = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, key)
        i = 0
        versions = []
        while True:
            try:
                name = _winreg.EnumKey(h, i)
            except WindowsError:
                break
            if is_version.match(name):
                versions.append(name)
            i += 1
        _winreg.CloseKey(h)
        h = None
        if not versions:
            msvc_fail('cannot parse registry')
        versions.sort(version.version_cmp)
        latest = versions[-1]
        h = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, key + '\\' + latest)
        i = 0
        subkeys = {}
        while True:
            try:
                skname, skvalue, sktype = _winreg.EnumValue(h, i)
            except WindowsError:
                break
            subkeys[skname] = skvalue
            i += 1
        _winreg.CloseKey(h)
        try:
            loc = subkeys['InstallationFolder']
        except KeyError:
            msvc_fail('cannot parse registry')
        _SDK_LOC = loc
        return loc
    finally:
        if h is not None:
            _winreg.CloseKey(h)

def get_sdk_environ(target, config):
    """Return the environment variables for the Windows SDK."""
    print
    dumpenv = False
    if dumpenv:
        print '==== Initial environment ===='
        for k, v in sorted(os.environ.items()):
            print k, '=', v
        print

    # Find the location of the latest Microsoft SDK
    sdk_loc = get_sdk_loc()
    if sdk_loc is None:
        msvc_fail('Windows SDK is not installed')
    setenv = os.path.join(sdk_loc, 'Bin', 'SetEnv.Cmd')
    if target not in ('x86', 'x64', 'ia64'):
        msvc_fail('target must be x86, x64, or ia64')
    if config not in ('debug', 'release'):
        msvc_fail('config must be debug or release')
    args = ' '.join(['/' + config, '/' + target, '/xp'])
    tag = 'TAG8306'
    # It seems that setenv gives an exit status of 1 normally
    cmd = 'cmd.exe /c "%s" %s & echo %s && set' % (setenv, args, tag)
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if proc.returncode:
        sys.stderr.write(out)
        sys.stderr.write(err)
        msvc_fail('setenv.cmd failed (return code=%d)' % proc.returncode)
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
    __slots__ = ['_dest', '_src', '_env', '_sourcetype', '_debugfile']

    def __init__(self, dest, src, env, sourcetype, debugfile):
        target.CC.__init__(self, dest, src, env, sourcetype)
        self._debugfile = debugfile

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
        if self._debugfile is not None:
            dflags = [('/Fd', self._debugfile)]
        else:
            dflags = []
        return [[cc, '/c', ('/Fo', self._dest), lflag] +
                [('/I', p) for p in env.CPPPATH] +
                list(env.CPPFLAGS) + list(warn) + list(cflags) +
                dflags + [self._src]]

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
        return [[env.LD, ('/OUT:', self._dest)] +
                list(env.LDFLAGS) + ds +
                self._src + list(env.LIBS)]

def add_sources(graph, proj, env, settings):
    pass

class VSFiles(target.Target):
    """Target for creating visual studio files."""
    __slots__ = ['_proj']

    def __init__(self, proj):
        self._proj = proj

    def input(self):
        return iter(())

    def output(self):
        return msvc.files_msvc(self._proj)

    def build(self, verbose):
        msvc.write_msvc(self._proj)
        return True

def add_targets(graph, proj, env, settings):
    deps = list(msvc.files_msvc(proj))
    deps.extend(graph.platform_built_sources(proj, 'WINDOWS'))
    graph.add(VSFiles(proj))
    graph.add(target.DepTarget('msvc', deps))
    if platform.system() == 'Windows':
        graph.add(target.DepTarget('default', ['msvc']))
        build_windows(graph, proj, env, settings)

def windows_env(env, settings):
    configs = {
        'debug':   ('/Od /Oy-', '/MDd', 'x86'),
        'release': ('/O2 /Oy-', '/MT',  'x86 x64'),
    }
    try:
        oflags, rtflags, archs = configs[settings.CONFIG]
    except KeyError:
        raise Exception('unsupported configuration: %r' % \
            (env.CONFIG,))

    # Get the user environment
    ccwarn = '/W3 /WX-'
    user_env = Environment(
        ARCHS=archs,
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
    user_env.update(env)

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
    base_env = Environment(
        CPPFLAGS=cppflags,
        CFLAGS=ccflags,
        CXXFLAGS=ccflags,
        LDFLAGS=ldflags,
        LIBS=libs,
    )

    return Environment(base_env, user_env)

def build_windows(graph, proj, env, settings):
    env = windows_env(env, settings)
    archs = env.ARCHS

    def lookup_env(atom):
        if atom in archs:
            aenv = Environment(LDFLAGS=['/MACHINE:' + atom])
            aenv.environ.update(get_sdk_environ(atom, settings.CONFIG))
            return aenv
        return None

    projenv = atom.ProjectEnv(proj, lookup_env, env)

    # Build the executable for each architecture
    apps = []
    types_cc = 'c', 'cxx'
    types_ignore = 'h', 'hxx'
    for targenv in projenv.targets('WINDOWS'):
        mname = targenv.simple_name
        appname = targenv.EXE_WINDOWS
        exes = []
        for arch in archs:
            targenv.arch = arch
            objs = []
            objdir = Path('build/obj-%s-%s' % (mname, arch))
            debugfile = Path(objdir, 'debug.pdb')
            def handlec(source, env):
                opath = Path(objdir, source.group.simple_name,
                             source.grouppath.withext('.obj'))
                objs.append(opath)
                graph.add(CC(opath, source.relpath, env,
                             source.sourcetype, debugfile))
            handlers = {}
            for t in types_cc: handlers[t] = handlec
            for t in types_ignore: handlers[t] = None
            targenv.apply(handlers)

            if arch == 'x86':
                exename = appname
            else:
                exename = '%s-%s' % (appname, arch)
            env = Environment(
                targenv.unionenv(),
                LDFLAGS=['/SUBSYSTEM:WINDOWS'],
            )
            pdbpath = Path('build/product', exename + '.pdb')
            exepath = Path('build/product', exename + '.exe')
            graph.add(LD(exepath, objs, env, pdbpath))
            exes.append(exepath)
            exes.append(pdbpath)

        pseudo = 'build-%s' % (appname,)
        graph.add(target.DepTarget(pseudo, exes))
        apps.append(pseudo)

    graph.add(target.DepTarget('build', apps))
