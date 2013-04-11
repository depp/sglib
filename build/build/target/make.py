from build.error import ConfigError
import importlib

MODNAME = {
    'linux': 'make_nix',
    'osx': 'make_osx',
}

def target(subtarget, os, cfg, args):
    if subtarget is None:
        subtarget = os
    try:
        modname = MODNAME[subtarget]
    except KeyError:
        raise ConfigError('invalid make subtarget: {!r}'.format(subtarget))
    mod = importlib.import_module('build.target.' + modname)
    return mod.Target(subtarget, cfg, args)
