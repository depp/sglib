import subprocess
import os
import re
from gen.error import ConfigError

__all__ = ['getproc', 'getoutput', 'run', 'escape']

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
        raise ConfigError('could not find the "%s" program' % name)
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
    return '\\x%02x' % i
def escape(x):
    """Escape a string for use in the shell."""
    if _SHELL_SPECIAL.search(x):
        return '"' + _SHELL_QSPECIAL.sub(escape1, x) + '"'
    if not x:
        return "''"
    return x
