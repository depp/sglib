#!/usr/bin/env python
import buildtool.tool
import os
import sys

tool = buildtool.tool.Tool()
tool.rootdir(os.path.dirname(sys.path[0]))
tool.srcdir('src')

tool.includepath('.')
tool.srclist('srclist-base.txt')
tool.srclist('srclist-client.txt')
for x in os.listdir('src/game'):
    if x.startswith('.'):
        continue
    if not os.path.isdir(os.path.join('src/game', x)):
        continue
    p = os.path.join('game', x, 'srclist.txt')
    if not os.path.isfile(os.path.join('src', p)):
        continue
    tool.srclist(p)

tool.run()
