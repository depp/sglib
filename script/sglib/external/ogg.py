# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule
from d3build.error import try_config
from d3build.external import ConfigureMake
import os

def pkg_config(build):
    return None, [], {'public': [build.env.pkg_config('ogg')]}

def bundled(build):
    env = build.env
    path = env.find_library('^libogg-[0-9.]+$')
    if build.config.target == 'msvc':
        project = os.path.join(
            path, 'win32', 'VS2010', 'libogg_static.vcxproj')
        varsets = [
            env.project_reference(project),
            env.header_path(os.path.join(path, 'include'), system=True),
        ]
    else:
        target = build.target.external_target(
            ConfigureMake(path), 'ogg')
        varsets = [
            env.header_path(
                os.path.join(target.destdir, 'include'),
                system=True),
            env.library(os.path.join(target.destdir, 'lib', 'libogg.a')),
        ]
        build.target.add_external_target(target)
    return path, [], {'public': [env.schema.merge(varsets)]}

def configure(build):
    return try_config([build], pkg_config, bundled)

module = ExternalModule(
    name='Ogg bitstream library',
    configure=configure,
    packages={
        'deb': 'libogg-dev',
        'rpm': 'libogg-devel',
        'gentoo': 'media-libs/libogg',
        'arch': 'libogg',
    }
)
