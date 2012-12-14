"""OS X generic environment."""
from gen.error import ConfigError
from gen.env.env import parse_env, merge_env

def osx_config(obj):
    if obj.srcname == 'framework':
        return {'LIBS': ('-framework', obj.name)}
    raise ConfigError('{} cannot be satisfied on the OS X generic target'
                      .format(obj.name))

def default_env(config, os):
    """Get the default environment."""
    assert os == 'OSX'
    envs = []
    if config.opts.get('warnings', True):
        envs.append({
            'CWARN': tuple(
                '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                '-Wwrite-strings -Wmissing-prototypes '
                .split()),
            'CXXWARN': tuple(
                '-Wall -Wextra -Wpointer-arith -Wno-sign-compare '
                .split()),
        })
    envs.append({
        'LDFLAGS': ('-Wl,-dead_strip', '-Wl,-dead_strip_dylibs'),
    })
    envs.append(parse_env(config.vars))
    return merge_env(envs)
