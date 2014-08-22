# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from build.error import ConfigError
import importlib

MODNAME = {
    'linux': 'nix',
    'osx': 'osx',
}

def target(subtarget, os, cfg, vars, archs):
    if subtarget is None:
        subtarget = os
    try:
        modname = MODNAME[subtarget]
    except KeyError:
        raise ConfigError('invalid make subtarget: {!r}'.format(subtarget))
    mod = importlib.import_module('build.target.make.' + modname)
    return mod.Target(subtarget, cfg, vars, archs)
