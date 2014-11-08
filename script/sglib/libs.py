# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.error import ConfigError
import os

WIN_ARCHMAP = {'Win32': 'x86', 'x64': 'x64'}

def binary_lib(build, path, libs):
    """Get a module for a library distributed in a certain standard format.

    The library is distributed in a folder containing an "include" and "lib"
    folder, and the "lib" folder contains a folder for each architecture.
    """
    if not build.config.platform == 'windows':
        raise ConfigError('binary_lib not supported on this platform')
    mod = build.target.module()
    for lib in libs:
        mod.add_library(lib)
    incpath = os.path.join(path, 'include')
    if not os.path.isdir(incpath):
        raise ConfigError('directory does not exist: {}'.format(incpath))
    mod.add_header_path(incpath, system=True)
    for arch in build.target.schema.archs:
        name = WIN_ARCHMAP[arch]
        libdir = os.path.join(path, 'lib', name)
        for lib in libs:
            libpath = os.path.join(libdir, lib)
            if not os.path.exists(libpath):
                raise ConfigError(
                    'library does not exist: {}'.format(libpath))
        mod.add_library_path(libdir, archs=[arch])
    return path, mod
