import platform
import pickle
import subprocess
import os
import sys

try:
    import _winreg
except ImportError:
    pass

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

def run_linux():
    pass

def run_macosx():
    pass

def setup_msvc_env():
    """Use the environment variables for Microsoft Visual C++."""
    def fail(why):
        print >>sys.stderr, 'Cannot find MSVC environment: %s' % why
        sys.exit(1)
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

def run_windows():
    setup_msvc_env()
    proc = subprocess.Popen(
        'cl.exe',
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = proc.communicate()
    print out
    print err
    print proc.returncode

SYSTEMS = {
    'Linux': run_linux,
    'Darwin': run_macosx,
    'Windows': run_windows,
}

def run():
    s = platform.system()
    try:
        f = SYSTEMS[s]
    except KeyError:
        print >>sys.stderr, 'Unknown platform: %r' % s
        sys.exit(1)
    else:
        f()

run()
