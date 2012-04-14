#!/usr/bin/env python
import os
import sys
root = sys.path[0]
sglib = os.path.join(os.path.dirname(os.path.dirname(root)), 'sglib')
sys.path.insert(1, os.path.abspath(os.path.join(sglib, 'scripts')))
from gen.project import *
p = Project(root)

p.info.set(
    PKG_NAME = 'Image test',
    PKG_IDENT = 'us.moria.img-test',
    PKG_EMAIL = 'depp@zdome.net',
    PKG_URL = 'http://moria.us/',
    PKG_COPYRIGHT = u'Copyright \xa9 2011-2012 Dietrich Epp',
)

p.add_module(Executable(
    'MAIN', 'Image Test executable',
    reqs = 'SGLIB',
    EXE_NAME = 'Image Test',
))

p.add_sourcelist_str('Image Test', '.', """\
image.c
""", 'MAIN')

p.run()
