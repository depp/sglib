import subprocess
import os
from gen.error import BuildError

PROC_CACHE = {}
def getproc(name):
    global PROC_CACHE
    try:
        return PROC_CACHE[name]
    except KeyError:
        pass
    for path in os.getenv('PATH').split(os.path.pathsep):
        if not path:
            continue
        fullpath = os.path.join(path, name)
        if os.access(fullpath, os.R_OK | os.X_OK):
            break
    else:
        raise BuildError('could not find the "%s" program' % name)
    PROC_CACHE[name] = fullpath
    return fullpath

def getoutput(cmd, **kw):
    """Get the output from running a command."""
    efile = getproc(cmd[0])
    proc = subprocess.Popen(
        cmd, executable=efile,
        stdout=subprocess.PIPE, **kw)
    stdout, stderr = proc.communicate()
    if proc.returncode:
        raise subprocess.CalledProcessError(proc.returncode, cmd)
    return stdout

def run(obj, cmd, **kw):
    print ' '.join(cmd)
    subprocess.check_call(cmd, **kw)
