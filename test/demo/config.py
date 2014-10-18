#!/usr/bin/env python3
import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', 'script'))
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

app = sglib.App(
    name='SGLib Demo',
    email='depp@zdome.net',
    uri='http://www.moria.us/',
    copyright='Copyright © 2011-2014 Dietrich Epp',
    identifier='us.moria.sglib-test',
    uuid='8d7f256e-ed27-443c-b350-c375d25a9e54',
    sources=src,
)

if __name__ == '__main__':
    app.configure()
