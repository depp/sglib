from .gensource import GeneratedSource, HEADER
import build.git as git
import re

ILLEGAL_CHAR = re.compile('[^ -~]')
ESCAPE_CHAR = re.compile(r'[\\"]')
def escape(s):
    m = ILLEGAL_CHAR.search(s)
    if m:
        raise ValueError('string contains illegal character: {!r}'
                         .format(m.group(0)))
    return ESCAPE_CHAR.sub(lambda x: '\\' + x.group(0), s)

class VersionInfo(GeneratedSource):
    __slots__ = ['repos', 'git']
    is_phony = True

    def __init__(self, name, target, repos, git):
        super(VersionInfo, self).__init__(name, target)
        self.repos = repos
        self.git = git

    def write(self, fp):
        fp.write('/* {}  */\n'.format(HEADER))
        for k, v in sorted(self.repos.items()):
            sha1, version = git.get_info(v, git=self.git)
            fp.write(
                'const char {}_VERSION[] = "{}";\n'
                'const char {}_COMMIT[] = "{}";\n'
                .format(k, escape(version), k, escape(sha1)))
