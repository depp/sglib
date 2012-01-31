import gen.path as path
import gen.build.target as target
from gen.env import Environment
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

def get_sdk_environ(obj, target, config):
    """Return the environment variables for the Windows SDK."""
    if obj.opts.dump_env:
        print '==== Initial environment ===='
        for k, v in sorted(os.environ.items()):
            print k, '=', v
        print
    if 0:
        p = getreg(r'SOFTWARE\Microsoft\VisualStudio\10.0\Setup\VC',
                   'ProductDir')
        if p is None:
            msvc_fail('VS 10 not installed')
        p = os.path.join(p, 'vcvarsall.bat')
    else:
        p = getreg(r'SOFTWARE\Microsoft\Microsoft SDKs\Windows',
                   'CurrentInstallFolder')
        if p is None:
            msvc_fail('Windows SDK is not installed')
        p = os.path.join(p, 'Bin', 'SetEnv.Cmd')
        if target not in ('x86', 'x64', 'ia64'):
            msvc_fail('--target must be x86, x64, or ia64')
        if config not in ('debug', 'release'):
            msvc_fail('--config must be debug or release')
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
        if obj.opts.dump_env:
            print line
        if line == tag:
            break
    else:
        msvc_fail('vcvarsall.bat failed (formatting)')
    env = {}
    for line in lines:
        k, v = line.rstrip().split('=', 1)
        env[k] = v
    if obj.opts.dump_env:
        print '==== MSVC Environment ===='
        for k, v in sorted(env.items()):
            print k, '=', v
        print
    return env

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
    return target.Command(cmd, inputs=[src], outputs=[obj],
                          env=env.environ, name=what)

def ld(obj, src, env):
    cmd = [env.LD, '/OUT:' + obj] + env.LDFLAGS + src + env.LIBS
    return target.Command(cmd, inputs=src, outputs=[obj],
                          env=env.environ, name='LD')

def build(obj):
    build = target.Build()

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
        ARCH='x86 x64',
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

    for arch in userenv.ARCH:
        objdir = os.path.join('build', 'obj-' + arch)
        cflags=['/Fd' + os.path.join(objdir, 'debug.pdb')]
        archenv = Environment(
            LDFLAGS=['/MACHINE:' + arch],
            CFLAGS=cflags,
            CXXFLAGS=cflags,
        )
        archenv.environ.update(get_sdk_environ(obj, arch, obj.opts.config))
        env = Environment(baseenv, archenv, userenv)

        exename = userenv.EXE_WINDOWS
        if arch != 'x86':
            exename = exename + '-' + arch

        # Build the sources
        objs = []
        for src in obj.get_atoms(None, 'WINDOWS', native=True):
            sbase, sext = os.path.splitext(src)
            stype = path.EXTS[sext]
            if stype in ('c', 'cxx'):
                objf = os.path.join(objdir, sbase + '.obj')
                build.add(cc(objf, src, env, stype))
                objs.append(objf)

        # Build the executable
        ldflags = ['/SUBSYSTEM:WINDOWS',
                   '/PDB:' + os.path.join('build', 'product', exename + '.pdb')]
        build.add(ld(os.path.join('build', 'product', exename + '.exe'),
                     objs, Environment(env, LDFLAGS=ldflags)))

    return build

