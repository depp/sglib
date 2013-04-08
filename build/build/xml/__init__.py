import build.xml.env
import build.data as data

from . import lxml as impl

_parse = impl.parse_file
dump = impl.dump

class Info(object):
    __slots__ = ['proj', 'base']
    def __init__(self, proj, base):
        self.proj = proj
        self.base = base
    def __getitem__(self, index):
        return self.proj.environ[index]

def parse_file(proj, path):
    if path.base != 'srcdir':
        raise ValueError('wrong base')
    dirname, basename = path.split()
    if not basename:
        raise ValueError('expected file, not directory name')
    buildfile = data.BuildFile()
    _parse(proj.native(dirname.join(basename + '.xml')),
           build.xml.env.RootEnv(
               Info(proj, path), dirname, buildfile))
    return buildfile
