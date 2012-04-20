#!/usr/bin/env python
import os
import sys
root = sys.path[0]
sglib = os.path.join(root, 'sglib')
sys.path.insert(1, os.path.join(sglib, 'scripts'))
from gen.project import *
p = Project(root)

p.info.set(
    PKG_NAME = 'SGLib Tests',
    PKG_IDENT = 'us.moria.img-test',
    PKG_EMAIL = 'depp@zdome.net',
    PKG_URL = 'http://moria.us/',
    PKG_COPYRIGHT = u'Copyright \xa9 2011-2012 Dietrich Epp',
    DEFAULT_CVARS = [
        ('log.level.root', 'debug'),
        ('log.winconsole', 'yes'),
        ('path.data-path', Path('data')),
    ]
)

p.add_module(Executable(
    'IMAGE', 'Image Test executable',
    reqs = 'SGLIB',
    EXE_NAME = 'Image Test',
))

p.add_module(Executable(
    'TYPE', 'Type (font) test executable',
    reqs = 'SGLIB',
    EXE_NAME = 'Type Test',
))

p.add_sourcelist_str('SGLib Tests', 'src', """\
image.c IMAGE
type.c TYPE
""")

p.run()
