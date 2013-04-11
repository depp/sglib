from . import GeneratedSource
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
    __slots__ = ['repos']
    is_regenerated_always = True

    def write(self, fp, cfg):
        fp.write('/* {}  */\n'.format(self.HEADER))
        for k, v in sorted(self.repos.items()):
            sha1, version = git.get_info(cfg, v)
            fp.write(
                'const char {}_VERSION[] = "{}";\n'
                'const char {}_COMMIT[] = "{}";\n'
                .format(k, escape(version), k, escape(sha1)))

    @classmethod
    def parse(class_, build, mod):
        repos = {}
        for k in mod.info:
            if k.startswith('repo.'):
                repos[k[5:]] = mod.info.get_path(k)
        return class_(
            target=mod.info.get_path('target'),
            repos=repos,
        )
