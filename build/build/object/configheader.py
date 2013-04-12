from . import GeneratedSource

def macro(x):
    return x.replace('-', '_').upper()

class ConfigHeader(GeneratedSource):
    __slots__ = ['bundles']

    def write(self, fp, cfg):
        fp.write(
            '/* {}  */\n'
            '#ifndef CONFIG_H\n'
            '#define CONFIG_H\n'
            '\n'
            '/* Enabled / disabled features */\n'
            .format(self.HEADER))
        for k, v in sorted(cfg.flags.items()):
            if v != 'no':
                print('#define ENABLE_{} 1'.format(macro(k)), file=fp)
            else:
                print('#undef  ENABLE_{}'.format(macro(k)), file=fp)
        fp.write(
            '\n'
            '/* Enabled feature implementations */\n')
        for k, v in sorted(cfg.flags.items()):
            if v == 'no' or v == 'yes':
                continue
            print('#define USE_{}_{} 1'.format(macro(k), macro(v)), file=fp)
        fp.write(
            '\n'
            '/* Bundled libraries */\n')
        for bundle in sorted(self.bundles):
            print('#define USE_BUNDLED_{} 1'.format(macro(bundle)), file=fp)
        fp.write(
            '\n'
            '#endif\n')

    @classmethod
    def parse(class_, build, mod):
        return class_(
            target=mod.info.get_path('target'),
            bundles=build.bundles,
        )
