import subprocess

def getoutput(cmd, **kw):
    """Get the output from running a command."""
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, **kw)
    stdout, stderr = proc.communicate()
    if proc.returncode:
        raise subprocess.CalledProcessError(proc.returncode, cmd)
    return stdout
