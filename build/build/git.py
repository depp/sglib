from build.shell import get_output
from build.error import format_block
import sys

def get_info(path, *, git='git'):
    """Get the version number and SHA-1 of the given Git repository.

    Returns (sha1, version).
    """
    stdout, stderr, retcode = get_output(
        [git, 'rev-parse', 'HEAD'], cwd=path)
    if retcode:
        sys.stderr.write(
            'warning: could not get SHA-1 of {}\n'.format(path.native))
        sys.stderr.write(format_block(stderr))
        return '<none>', '0.0'
    sha1 = stdout.strip()

    stdout, stderr, retcode = get_output(
        ['git', 'describe'], cwd=path)
    if retcode:
        sys.stderr.write(
            'warning: could not get version number in {}\n'
            .format(path.native))
        sys.stderr.write(format_block(stderr))

        stdout, stderr, retcode = get_output(
            ['git', 'rev-list', 'HEAD'], cwd=path)
        if retcode:
            sys.stderr.write(
                'warning: could not get list of git revisions in {}\n'
                .format(path.native))
            sys.stderr.write(format_block(stderr))
            version = '0.0'
        else:
            nrev = len(stdout.splitlines())
            version = '0.0-{}-g{}'.format(nrev, sha1[:7])
    else:
        version = stdout.strip()

    return sha1, version
