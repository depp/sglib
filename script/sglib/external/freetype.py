# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.error import ConfigError
from d3build.generatedsource.configuremake import ConfigureMake
from d3build.package import ExternalPackage
from ..libs import  binary_lib
import os

def pkg_config(build):
    flags = build.pkg_config('freetype2')
    return None, build.target.module().add_flags(flags)

def find_package(build):
    return build.find_package('^freetype(?:-[0-9.]+)$')

def freetype_version(path):
    name = os.path.basename(path)
    i = name.find('-')
    if i < 0:
        return None
    ver = name[i+1:].replace('.', '')
    if not ver.isnumeric():
        return None
    return ver

def bundled_bin(build):
    if build.config.platform != 'windows':
        raise ConfigError('this only works on Windows')
    path = find_package(build)
    ver = freetype_version(path)
    if ver is None:
        raise ConfigError('cannot decipher FreeType version')
    return binary_lib(build, path, ['freetype{}MT.lib'.format(ver)])

CONFIG_ARGS = '''
--prefix=/
--disable-shared
--enable-static
--without-bzip2
--without-old-mac-fonts
--without-fsspec
--without-fsref
--without-quickdraw-toolbox
--without-quickdraw-carbon
--without-ats
'''.split()

def bundled_src(build):
    path = find_package(build)
    if build.config.target == 'msvc':
        raise ConfigError('not supported')
    target = ConfigureMake(build, 'freetype', path, args=CONFIG_ARGS)
    mod = (build.target.module()
        .add_generated_source(target)
        .add_header_path(
            os.path.join(target.destdir, 'include', 'freetype2'),
            system=True)
        .add_library(os.path.join(target.destdir, 'lib', 'libfreetype.a'))
        .add_library('-lz'))
    return path, mod

module = ExternalPackage(
    [pkg_config, bundled_bin, bundled_src],
    name='FreeType library',
    packages={
        'deb': 'libfreetype6-dev',
        'rpm': 'freetype-devel',
        'gentoo': 'media-libs/freetype',
        'arch': 'freetype2',
    }
)
