from .gensource import GeneratedSource, HEADER

class ConfigHeader(GeneratedSource):
    __slots__ = ['flags']

    def __init__(self, name, target, flags):
        super(ConfigHeader, self).__init__(name, target)
        self.flags = flags

    @property
    def deps(self):
        return ()

    def write(self, fp):
        fp.write(
            '/* {}  */\n'
            '#ifndef CONFIG_H\n'
            '#define CONFIG_H\n'
            '\n'
            '/* Enabled / disabled features */\n'
            .format(HEADER))
        for k, v in sorted(self.flags.items()):
            k = k.replace('-', '_').upper()
            if v != 'no':
                print('#define ENABLE_{} 1'.format(k), file=fp)
            else:
                print('#undef  ENABLE_{}'.format(k), file=fp)
        fp.write(
            '\n'
            '/* Enabled feature implementations */\n')
        for k, v in sorted(self.flags.items()):
            if v == 'no' or v == 'yes':
                continue
            k = k.replace('-', '_').upper()
            v = v.replace('-', '_').upper()
            print('#define USE_{}_{} 1'.format(k, v), file=fp)
        fp.write(
            '\n'
            '#endif\n')
