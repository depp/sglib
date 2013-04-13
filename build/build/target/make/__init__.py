from build.error import ConfigError
import importlib

MODNAME = {
    'linux': 'nix',
    'osx': 'osx',
}

def target(subtarget, os, cfg, args, archs):
    if subtarget is None:
        subtarget = os
    try:
        modname = MODNAME[subtarget]
    except KeyError:
        raise ConfigError('invalid make subtarget: {!r}'.format(subtarget))
    mod = importlib.import_module('build.target.make.' + modname)
    return mod.Target(subtarget, cfg, args, archs)
