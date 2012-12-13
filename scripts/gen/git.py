from gen.shell import getproc
from gen.error import ConfigError, format_block
import subprocess
import sys

def get_info(config, path):
    """Get the version number and SHA-1 of the given Git repository.

    Returns (sha1, version).
    """
    procname = config.vars.get('GIT', 'git')
    exe = getproc(procname)

    def git(*args):
        cmd = [procname]
        cmd.extend(args)
        proc = subprocess.Popen(
            cmd, executable=exe, cwd=path.native,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = proc.communicate()
        return stdout, stderr, proc.returncode

    stdout, stderr, retcode = git('rev-parse', 'HEAD')
    if retcode:
        sys.stderr.write(
            'warning: could not get SHA-1 of %s\n' % path.native)
        sys.stderr.write(format_block(stderr))
        return '<none>', '0.0'
    sha1 = stdout.strip()

    stdout, stderr, retcode = git('describe')
    if retcode:
        sys.stderr.write(
            'warning: could not get version number in %s\n' % path.native)
        sys.stderr.write(format_block(stderr))

        stdout, stderr, retcode = git('rev-list', 'HEAD')
        if retcode:
            sys.stderr.write(
                'warning: could not get list of git revisions in %s\n'
                % path.native)
            sys.stderr.write(format_block(stderr))
            version = '0.0'
        else:
            nrev = len(stdout.splitlines())
            version = '0.0-%d-g%s' % (nrev, sha1[:7])
    else:
        version = stdout.strip()

    return sha1, version
