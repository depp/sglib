import subprocess
import os
import re
from build.error import ConfigError, format_block

__all__ = ['getproc', 'getoutput', 'run', 'escape']

PROC_CACHE = {}
def find_exe(name):
    """Find the given executable, or return None if not found."""
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
        return None
    PROC_CACHE[name] = fullpath
    return fullpath

# POSIX: special are |&;<>()$\\\"' *?[#~=%
#        non-special are !+,-./:@]^_`{}
# We escape some ones that are unnecessary
_SHELL_SPECIAL = re.compile(r'[^-A-Za-z0-9+,./:@^_]')
_SHELL_QSPECIAL = re.compile('["\\\\]')
def escape1(x):
    x = x.group(0)
    i = ord(x)
    if 32 <= i <= 126:
        return '\\' + x
    return '\\x{:02x}'.format(i)
def escape(x):
    """Escape a string for use in the shell."""
    if _SHELL_SPECIAL.search(x):
        return '"' + _SHELL_QSPECIAL.sub(escape1, x) + '"'
    if not x:
        return "''"
    return x

def escape_cmd(cmd):
    return ' '.join(escape(x) for x in cmd)

def get_output(cmd, cwd=None, combine_output=False):
    """Run a command.

    If combine_output is True, then returns (stdout, returncode).

    Otherwise, returns (stdout, stderr, returncode).

    Raises a ConfigError if the command is not found.
    """
    exe = find_exe(cmd[0])
    if exe is None:
        raise ConfigError('could not find {} command'.format(cmd[0]))
    proc = subprocess.Popen(
        cmd,
        executable=exe, cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT if combine_output else subprocess.PIPE)
    stdout, stderr = proc.communicate()
    try:
        stdout = stdout.decode()
        if combine_output:
            return stdout, proc.returncode
        else:
            stderr = stderr.decode()
            return stdout, stderr, proc.returncode
    except UnicodeDecodeError:
        raise ConfigError(
            'could not parse {} output'.format(cmd[0]),
            'command: {}'.format(escape_cmd(cmd)))

def describe_proc(cmd, output, retcode):
    return (
        'command: {}\n'
        'status: {}\n'
        'output:\n'
        '{}'
    ).format(escape_cmd(cmd), retcode, output)
