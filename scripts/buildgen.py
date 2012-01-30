#!/usr/bin/env python
import buildtool.tool
import os
import sys

tool = buildtool.tool.Tool()
tool.rootdir(os.path.dirname(sys.path[0]))

tool.env.PKG_NAME = 'My Game'
tool.env.PKG_IDENT = 'us.moria.my-game'
tool.env.EXE_NAME = 'Game'

tool.includepath('src')
tool.srclist(os.path.join('src', 'srclist-base.txt'))
tool.srclist(os.path.join('src', 'srclist-client.txt'))
r = os.path.join('src', 'game')
for x in os.listdir(r):
    if x.startswith('.'):
        continue
    p = os.path.join(r, x)
    if not os.path.isdir(p):
        continue
    p = os.path.join(p, 'srclist.txt')
    if not os.path.isfile(p):
        continue
    tool.srclist(p)

tool.run()
