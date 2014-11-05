#!/usr/bin/env python3
# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import sys
from os.path import join, dirname
sys.path.append(join(dirname(__file__), '..', '..', 'script'))
import sglib

src = sglib.SourceList(base=__file__, path='src', sources='''
defs.h
load_audio.c
load_program.c
load_shader.c
load_texture.c
main.c
menu.c
test_audio.c
test_image.c
test_type.c
text_layout.c
''')

icon = sglib.Icon(base=__file__, path='icon', sources='''
ico icon.ico
16 icon16.png
32 icon32.png
48 icon48.png
64 icon64.png
128 icon128.png
256 icon256.png
512 icon512.png
1024 icon1024.png
''')

app = sglib.App(
    name='SGLib Demo',
    datapath=sglib._base(__file__, 'data'),
    email='depp@zdome.net',
    uri='http://www.moria.us/',
    copyright='Copyright © 2011-2014 Dietrich Epp',
    identifier='us.moria.sglib-test',
    uuid='8d7f256e-ed27-443c-b350-c375d25a9e54',
    sources=src,
    icon=icon,
)

if __name__ == '__main__':
    app.run()
