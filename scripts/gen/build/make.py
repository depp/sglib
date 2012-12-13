from gen.error import ConfigError

OS_MAKEFILE = {
    'LINUX': 'linux',
    'OSX': 'osx',
}

def gen_makefile(config):
    native = config.native_os
    try:
        mod_name = OS_MAKEFILE[native]
    except KeyError:
        raise ConfigError('unsupported platform for building with makefiles')
    mod = __import__('gen.build.' + mod_name)
    mod = getattr(mod.build, mod_name)
    mod.gen_makefile(config)
