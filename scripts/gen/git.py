import gen.shell as shell
import subprocess
import re
import sys

GOOD_VERSION = re.compile(r'^[0-9]+(?:\.[0-9]+)(?:-.+)?$')

def git(env, path, *cmd):
    return shell.getoutput([env.GIT] + list(cmd), cwd=path)

def get_version(env, path):
    """Get the version number from the Git repository.

    This will use git-describe to get the version number.  If
    git-describe fails, then this will return 0.0.
    """
    try:
        desc = git(env, path, 'describe', '--abbrev=0')
    except subprocess.CalledProcessError:
        print >>sys.stderr, 'warning: no git tags found'
        return '0.0'
    desc = desc.strip()
    if not GOOD_VERSION.match(desc):
        print >>sys.stderr, \
            'warning: cannot parse version string: %s' % (desc,)
    return desc

def get_sha1(env, path):
    try:
        sha1 = git(env, path, 'rev-parse', 'HEAD')
    except subprocess.CalledProcessError:
        print >>sys.stderr, 'warning: no git HEAD found'
        return '0' * 40
    return sha1.strip()
