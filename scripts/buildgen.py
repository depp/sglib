#!/usr/bin/env python
import buildtool.tool
import os
import sys

tool = buildtool.tool.Tool()
tool.rootdir(os.path.dirname(sys.path[0]))

tool.includepath('src')
tool.srclist('src/srclist-base.txt')
tool.srclist('src/srclist-client.txt')
for x in os.listdir('src/game'):
    if x.startswith('.'):
        continue
    p = os.path.join('src/game', x)
    if not os.path.isdir(p):
        continue
    p = os.path.join(p, 'srclist.txt')
    if not os.path.isfile(p):
        continue
    tool.srclist(p)

tool.run()
