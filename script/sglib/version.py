# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.shell import get_output
from d3build.error import format_block
from d3build.generatedsource import GeneratedSource, NOTICE
import collections
import sys
import re

def warn(*msg):
    print(*msg, file=sys.stderr)

# 'number' is a triplet (x, y, z)
# 'desc' is the Git description 'vX.Y.Z-1-gSHA1'
# 'sha1' is the Git SHA-1
Version = collections.namedtuple('Version', 'number desc sha1')

VERSION_STRING = re.compile(
    r'v(\d+)(?:\.(\d+)(?:\.(\d+))?)?(?:-\d+(?:-.*)?)?')

def get_info(git, path):
    """Get the of the given Git repository."""
    stdout, stderr, retcode = get_output(
        [git, 'rev-parse', 'HEAD'], cwd=path)
    if retcode:
        warn(
            'could not get SHA-1 for repository: {}'.format(path),
            format_block(stderr))
        return Version((0, 0, 0), 'v0.0.0', '<none>')
    sha1 = stdout.strip()

    stdout, stderr, retcode = get_output(
        ['git', 'describe'], cwd=path)
    if retcode:
        warn(
            'could not get version number for repository: {}'.format(path),
            format_block(stderr))

        stdout, stderr, retcode = get_output(
            ['git', 'rev-list', 'HEAD'], cwd=path)
        if retcode:
            warn(
                'could not get list of git revisions for repository: {}'
                .format(path), format_block(stderr))
            version = 'v0.0.0'
        else:
            nrev = len(stdout.splitlines())
            version = 'v0.0.0-{}-g{}'.format(nrev, sha1[:7])
    else:
        version = stdout.strip()

    m = VERSION_STRING.match(version)
    if not m:
        warn('could not parse version number: {}'.format(version))
        number = 0, 0, 0
    else:
        number = tuple(int(x) if x is not None else 0 for x in m.groups())
    return Version(number, version, sha1)

class VersionInfo(GeneratedSource):
    __slots__ = ['_target', 'app_path', 'sglib_path', 'git']

    def __init__(self, target, app_path, sglib_path, git):
        self._target = target
        self.app_path = app_path
        self.sglib_path = sglib_path
        self.git = git

    @property
    def is_regenerated_always(self):
        return True

    @property
    def target(self):
        return self._target

    def write(self, fp):
        fp.write('/* {} */\n'.format(NOTICE))
        for name, path in (('APP', self.app_path), ('SG', self.sglib_path)):
            version = get_info(self.git, path)
            fp.write(
                'const char SG_{}_VERSION[] = "{}";\n'
                'const char SG_{}_COMMIT[] = "{}";\n'
                .format(name, version.desc, name, version.sha1))
