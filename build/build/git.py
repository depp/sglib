# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from build.shell import get_output
from build.error import format_block

def get_info(cfg, path):
    """Get the version number and SHA-1 of the given Git repository.

    Returns (sha1, version).
    """
    git = 'git' # FIXME
    path = cfg.native_path(path)
    stdout, stderr, retcode = get_output(
        [git, 'rev-parse', 'HEAD'], cwd=path)
    if retcode:
        cfg.warn(
            'could not get SHA-1 for repository: {}'.format(path),
            format_block(stderr))
        return '<none>', '0.0'
    sha1 = stdout.strip()

    stdout, stderr, retcode = get_output(
        ['git', 'describe'], cwd=path)
    if retcode:
        cfg.warn(
            'could not get version number for repository: {}'.format(path),
            format_block(stderr))

        stdout, stderr, retcode = get_output(
            ['git', 'rev-list', 'HEAD'], cwd=path)
        if retcode:
            cfg.warn(
                'could not get list of git revisions for repository: {}'
                .format(path), format_block(stderr))
            version = '0.0'
        else:
            nrev = len(stdout.splitlines())
            version = '0.0-{}-g{}'.format(nrev, sha1[:7])
    else:
        version = stdout.strip()

    return sha1, version
