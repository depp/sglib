# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.error import ConfigError
from d3build.generatedsource.configuremake import ConfigureMake
from d3build.package import ExternalPackage
from ..libs import  binary_lib
import os

def pkg_config(build):
    flags = build.pkg_config('opus')
    return None, build.target.module().add_flags(flags)

WIN_LIBNAMES = ['opus', 'celt', 'silk_common', 'silk_float']

def find_package(build):
    return build.find_package('^opus(?:-[0-9.]+)?$')

def bundled_bin(build):
    return binary_lib(build, find_package(build),
                      [name + '.lib' for name in WIN_LIBNAMES])

def bundled_src(build):
    path = find_package(build)
    if build.config.target == 'msvc':
        vs = os.path.join(path, 'win32', 'VS2010')
        mod = build.target.module()
        for name in ('opus', 'celt', 'silk_common', 'silk_float'):
            mod.add_vcxproj(os.path.join(vs, name + '.vcxproj'))
        mod.add_header_path(
            os.path.join(path, 'include'), system=True)
    else:
        target = ConfigureMake(
            build, 'opus', path,
            args=['--prefix=/', '--disable-shared', '--enable-static'])
        mod = (build.target.module()
            .add_generated_source(target)
            .add_header_path(
                os.path.join(target.destdir, 'include', 'opus'),
                system=True)
            .add_library(os.path.join(target.destdir, 'lib', 'libopus.a')))
    return path, mod

module = ExternalPackage(
    [pkg_config, bundled_bin, bundled_src],
    name='Opus codec library',
    packages={
        'deb': 'libopus-dev',
        'rpm': 'opus',
        'gentoo': 'media-libs/opus',
        'arch': 'opus',
    }
)
