from . import GeneratedSource

class ConfigHeader(GeneratedSource):
    __slots__ = []

    def write(self, fp, cfg):
        fp.write(
            '/* {}  */\n'
            '#ifndef CONFIG_H\n'
            '#define CONFIG_H\n'
            '\n'
            '/* Enabled / disabled features */\n'
            .format(self.HEADER))
        for k, v in sorted(cfg.flags.items()):
            k = k.replace('-', '_').upper()
            if v != 'no':
                print('#define ENABLE_{} 1'.format(k), file=fp)
            else:
                print('#undef  ENABLE_{}'.format(k), file=fp)
        fp.write(
            '\n'
            '/* Enabled feature implementations */\n')
        for k, v in sorted(cfg.flags.items()):
            if v == 'no' or v == 'yes':
                continue
            k = k.replace('-', '_').upper()
            v = v.replace('-', '_').upper()
            print('#define USE_{}_{} 1'.format(k, v), file=fp)
        fp.write(
            '\n'
            '#endif\n')

    @classmethod
    def parse(class_, build, mod):
        return class_(
            target=mod.info.get_path('target'),
        )
