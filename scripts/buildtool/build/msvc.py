import buildtool.path as path
import buildtool.build.target as target
from buildtool.env import Environment
import subprocess
import os
import sys
import _winreg

def getreg(key, subkey):
    """Get a value from the Windows registry."""
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

ENV_IS_SETUP = False

def setup_msvc_env():
    """Use the environment variables for Microsoft Visual C++."""
    global ENV_IS_SETUP
    if ENV_IS_SETUP:
        return
    p = getreg(r'SOFTWARE\Microsoft\VisualStudio\10.0\Setup\VC',
                'ProductDir')
    if p is None:
        fail('VS 10 not installed')
    p = os.path.join(p, 'vcvarsall.bat')
    tag = 'TAG8306'
    cmd = 'cmd.exe /c "%s" && echo %s && set' % (p, tag)
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if proc.returncode:
        fail('vcvarsall.bat failed (return code)')
    lines = iter(out.splitlines())
    for line in lines:
        line = line.rstrip()
        if line == tag:
            break
    else:
        fail('vcvarsall.bat failed (formatting)')
    env = {}
    for line in lines:
        k, v = line.rstrip().split('=', 1)
        os.environ[k] = v
    ENV_IS_SETUP = True

def cc(obj, src, env, stype):
    if stype == 'c':
        cc = env.CC
        cflags = env.CFLAGS
        warn = env.CWARN
        what = 'CC'
        lflag = '/TC'
    elif stype == 'cxx':
        cc = env.CXX
        cflags = env.CXXFLAGS
        warn = env.CWARN
        what = 'CXX'
        lflag = '/TP'
    else:
        raise ValueError('not a C file type: %r' % stype)
    cmd = [cc, '/c', '/Fo' + obj, lflag] + \
        env.CPPFLAGS + warn + cflags + [src]
    return target.Command(cmd, inputs=[src], outputs=[obj], name=what)

def ld(obj, src, env):
    cmd = [env.LD, '/OUT:' + obj] + env.LDFLAGS + src + env.LIBS
    return target.Command(cmd, inputs=src, outputs=[obj], name='LD')

def build(obj):
    build = target.Build()
    setup_msvc_env()

    # Get the base environment
    defs = ['_CRT_SECURE_NO_DEPRECATE', 'WIN32', 'DEBUG', '_WINDOWS']
    cppflags = ['/I' + p for p in obj.incldirs] + \
        ['/D' + deff for deff in defs]
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
        '/Fd' + os.path.join('build', 'obj', 'debug.pdb'),
    ]
    ldflags = [
        '/NOLOGO',      # don't print version info
        '/OPT:REF',     # dead code elimination
        '/OPT:ICF',     # identical constant fol/folding
        '/DYNAMICBASE', # allow ASLR
        '/NXCOMPAT',    # allow NX bit
        '/MACHINE:X86',
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
    baseenv = Environment(
        CPPFLAGS=cppflags,
        CFLAGS=ccflags,
        CXXFLAGS=ccflags,
        LDFLAGS=ldflags,
        LIBS=libs
    )
    ccwarn = ['/W3', '/WX-']
    if obj.opts.config == 'release':
        ccflags = ['/O2', '/Oy-', '/MT']
    elif obj.opts.config == 'debug':
        ccflags = ['/Od', '/Oy-', '/MDd']
    else:
        print >>sys.stderr, 'unsupported configuration: %r' % \
            (obj.opts.config,)
        sys.exit(1)
    userenv = Environment(
        CC='cl.exe',
        CXX='cl.exe',
        CPPFLAGS='',
        CFLAGS=ccflags,
        CXXFLAGS=ccflags,
        CWARN=ccwarn,
        CXXWARN=ccwarn,
        LD='link.exe',
        LDFLAGS='',
        LIBS='',
    )
    userenv.override(obj.env)

    exename = userenv.EXE_WINDOWS

    # Build the sources
    objs = []
    env = Environment(baseenv, userenv)
    objdir = os.path.join('build', 'obj')
    for src in obj.get_atoms(None, 'WINDOWS', native=True):
        sbase, sext = os.path.splitext(src)
        stype = path.EXTS[sext]
        if stype in ('c', 'cxx'):
            objf = os.path.join(objdir, sbase + '.obj')
            build.add(cc(objf, src, env, stype))
            objs.append(objf)

    # Build the executable
    ldflags = ['/SUBSYSTEM:WINDOWS',
               '/PDB:' + os.path.join('build', 'product', 'exename' + '.pdb')]
    build.add(ld(os.path.join('build', 'product', exename + '.exe'),
                 objs, Environment(env, LDFLAGS=ldflags)))

    return build

