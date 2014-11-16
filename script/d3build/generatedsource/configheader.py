# Copyright 2013-2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from . import GeneratedSource, NOTICE

def macro(x):
    return x.replace('-', '_').upper()

class ConfigHeader(GeneratedSource):
    __slots__ = ['target', 'flags', 'options']

    def __init__(self, target, build):
        self.target = target
        self.flags = build.config.flags
        self.options = build.options

    @property
    def dependencies(self):
        return ()

    def write(self, fp):
        fp.write(
            '/* {} */\n'
            '#ifndef CONFIG_H\n'
            '#define CONFIG_H\n'
            '\n'
            '/* Enabled / disabled features */\n'
            .format(NOTICE))

        def writevar(name, status):
            s = '#define ENABLE_{} 1'.format(name)
            if status:
                print(s, file=fp)
            else:
                print('/* {} */'.format(s), file=fp)

        for option in sorted(self.options, key=lambda x: x.name):
            print(file=fp)
            name = macro(option.name)
            value = self.flags[option.name]
            writevar(name, value and value != 'none')
            if not option.options:
                continue
            for optvalue in sorted(option.options):
                writevar('{}_{}'.format(name, macro(optvalue)),
                         value == optvalue)

        fp.write(
            '\n'
            '#endif\n')
