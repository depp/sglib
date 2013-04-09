from build.error import ConfigError

def Target(subtarget, os, args):
    if subtarget is None:
        subtarget = os
    if subtarget in ('linux', 'osx'):
        from . import gmake as mod
    else:
        raise ConfigError('invalid make subtarget: {!r}'.format(subtarget))
    return mod.Target(subtarget, os, args)
