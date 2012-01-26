from __future__ import with_statement
import os

def run(obj):
    inpath = os.path.join(os.path.dirname(__file__), 'gmake.txt')
    text = open(inpath, 'r').read()
    obj._write_file('src/Makefile', text)
