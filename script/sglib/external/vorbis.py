# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.error import ConfigError
from d3build.generatedsource.configuremake import ConfigureMake
from d3build.package import ExternalPackage
import os

def pkg_config(build):
    return None, build.target.module().add_pkg_config('vorbis')

def bundled(build):
    path = build.find_package('^libvorbis(:?-[0-9.]+)?$')
    if build.config.target == 'msvc':
        project = os.path.join(
            path, 'win32', 'VS2010', 'libvorbis', 'libvorbis_static.vcxproj')
        mod = (build.target.module()
            .add_vcxproj(project)
            .add_header_path(os.path.join(path, 'include'), system=True))
    else:
        target = ConfigureMake(
            build, 'vorbis', path,
            dependencies=['ogg'],
            args=['--prefix=/', '--disable-shared', '--enable-static',
                  '--with-ogg=' + os.path.abspath(build.external_destdir())])
        mod = (build.target.module()
            .add_generated_source(target)
            .add_header_path(
                os.path.join(target.destdir, 'include'),
                system=True)
            .add_library(os.path.join(target.destdir, 'lib', 'libvorbis.a')))
    return path, mod

module = ExternalPackage(
    [pkg_config, bundled],
    name='Vorbis codec library',
    packages={
        'deb': 'libvorbis-dev',
        'rpm': 'libvorbis-devel',
        'gentoo': 'media-libs/libvorbis',
        'arch': 'libvorbis',
    }
)
