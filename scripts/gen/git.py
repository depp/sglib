import gen.shell as shell
import subprocess
import re
import sys

def get_version(obj, path):
    """Get the current version as (X, Y, Z) where X, Y, Z are integers.

    If the repository is tagged, the most recent tag must be of the form
    X[.Y[.Z]], where X, Y, and Z are integers.  Missing values are replaced
    with 0.  If the tag has the wrong format or cannot be found, None is
    returned and a warning is printed.
    """
    git = obj.opts.git_path or 'git'
    try:
        desc = shell.getoutput([git, 'describe', '--abbrev=0'], cwd=path)
    except subprocess.CalledProcessError:
        print >>sys.stderr, 'warning: no git tags found'
        return
    desc = desc.strip()
    try:
        s = [int(x) for x in s.split('.')]
    except ValueError:
        print >>sys.stderr, 'warning: cannot parse git tag %r' % (desc,)
        return
    if len(s) > 3:
        print >>sys.stderr, 'warning: version too long: %r' % (desc,)
        s = s[:3]
    elif len(s) < 3:
        s = s + [0] * (3 - len(s))
    return tuple(s)

def get_sha1(obj, path):
    git = obj.opts.git_path or 'git'
    try:
        sha1 = shell.getoutput([git, 'rev-parse', 'HEAD'], cwd=path)
    except subprocess.CalledProcessError:
        print >>sys.stderr, 'warning: no git HEAD found'
        return None
    return sha1.strip()
