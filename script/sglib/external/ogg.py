# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.error import ConfigError
from d3build.generatedsource.configuremake import ConfigureMake
from d3build.package import ExternalPackage
from ..libs import  binary_lib
import os

def pkg_config(build):
    flags = build.pkg_config('ogg')
    return None, build.target.module().add_flags(flags)

def find_package(build):
    return build.find_package('^libogg(?:-[0-9.]+)?$')

def bundled_bin(build):
    return binary_lib(build, find_package(build), ['libogg_static.lib'])

def bundled_src(build):
    path = find_package(build)
    if build.config.target == 'msvc':
        project = os.path.join(
            path, 'win32', 'VS2010', 'libogg_static.vcxproj')
        mod = (build.target.module()
            .add_vcxproj(project)
            .add_header_path(os.path.join(path, 'include'), system=True))
    else:
        target = ConfigureMake(
            build, 'ogg', path,
            args=['--prefix=/', '--disable-shared', '--enable-static'])
        mod = (build.target.module()
            .add_generated_source(target)
            .add_header_path(
                os.path.join(target.destdir, 'include'),
                system=True)
            .add_library(os.path.join(target.destdir, 'lib', 'libogg.a')))
    return path, mod

module = ExternalPackage(
    [pkg_config, bundled_bin, bundled_src],
    name='Ogg bitstream library',
    packages={
        'deb': 'libogg-dev',
        'rpm': 'libogg-devel',
        'gentoo': 'media-libs/libogg',
        'arch': 'libogg',
    }
)
