# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from . import GeneratedSource, NOTICE

def macro(x):
    return x.replace('-', '_').upper()

class ConfigHeader(GeneratedSource):
    __slots__ = ['target', 'flags']

    def __init__(self, target, env):
        self.target = target
        self.flags = env.flags._vars

    @property
    def dependencies(self):
        return ()

    def write(self, fp):
        fp.write(
            '/* {}  */\n'
            '#ifndef CONFIG_H\n'
            '#define CONFIG_H\n'
            '\n'
            '/* Enabled / disabled features */\n'
            .format(NOTICE))
        for k, v in sorted(self.flags.items()):
            k = macro(k)
            if isinstance(v, bool):
                e = '#define ENABLE_{} 1'.format(k)
                if v:
                    print(e, file=fp)
                else:
                    print('/* {} */'.format(e), file=fp)
            else:
                print('#define ENABLE_{}_{} 1'.format(k, macro(v)), file=fp)
        fp.write(
            '\n'
            '#endif\n')
