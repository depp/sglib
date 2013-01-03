import platform
import gen.util as util
from gen.error import ConfigError

# Modules containing default targets for each platform
os_map = {
    'Darwin': 'osx',
    'Windows': 'windows',
    'Linux': 'linux',
}

def get_target(name):
    i = name.find('.')
    if i >= 0:
        targ, subtarg = name[:i], name[i+1:]
    else:
        targ = name
        subtarg = 'default'
    if not util.is_ident(targ) and util.is_ident(subtarg):
        raise ConfigError('invalid target: {}'.format(name))
    if targ == 'default':
        os = platform.system()
        try:
            targ = os_map[os]
        except KeyError:
            raise ConfigError('unknown operating system: {}'.format(os))
    try:
        m = __import__('gen.target.' + targ)
    except ImportError:
        pass
    else:
        m = getattr(m.target, targ)
        if subtarg == 'default':
            try:
                subtarg = m.TARGET_DEFAULT
            except AttributeError:
                subtarg = None
        try:
            if subtarg is not None:
                func = getattr(m, 'target_' + subtarg)
        except AttributeError:
            pass
        else:
            return func
    raise ConfigError('no such target: {}'.format(name))
