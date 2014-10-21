# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.source import SourceList, _base
# Note: SourceList is for export
from d3build.build import Build
from d3build.module import SourceModule
from . import options
from . import module
import sys
import os

class App(object):
    __slots__ = [
        # Name of the project.
        'name',
        # SourceList of the project source code.
        'sources',
        # Path to the project data files.
        'datapath',
        # Contact email address.
        'email',
        # Project home page URI.
        'uri',
        # Project copyright notice.
        # Should be "Copyright © YEAR Author".
        'copyright',
        # Project identifier reverse domain name notation, for OS X.
        'identifier',
        # Project UUID, for emitting Visual Studio projects (optional).
        'uuid',
        # Default flag values.
        'defaults',
        # Function for configuring the main project module (optional).
        'configure_func',
    ]

    def __init__(self, *, name, sources, datapath,
                 email=None, uri=None, copyright=None,
                 identifier=None, uuid=None, defaults=None,
                 configure=None):
        self.name = name
        self.sources = sources
        self.datapath = datapath
        self.email = email
        self.uri = uri
        self.copyright = copyright
        self.identifier = identifier
        self.uuid = uuid
        self.defaults = defaults
        self.configure_func = configure

    def run(self):
        """Run the configuration script."""
        Build.run(
            name=self.name,
            build=self._build,
            sources=self.sources.sources + module.module.sources,
            options=options.flags,
            adjust_config=self._adjust_config,
        )

    def _module_configure(self, build):
        """Get the configuration for the main application module."""
        if self.configure_func is None:
            tags = {}
        else:
            tags = dict(self.configure_func(build))
        private = list(tags.get('private', []))
        private.append(module.module)
        tags['private'] = private
        return tags

    def _adjust_config(self, cfg):
        """Adjust the configuration."""
        for defaults in (self.defaults, options.defaults):
            if not defaults:
                continue
            for platform in (cfg.platform, None):
                try:
                    flags = defaults[platform]
                except KeyError:
                    continue
                for flag, value in flags.items():
                    if flag not in cfg.flags:
                        raise ValueError('unknown flag: {!r}'.format(flag))
                    if cfg.flags[flag] is None:
                        cfg.flags[flag] = value

    def _build(self, build):
        """Create the project targets."""
        from .version import VersionInfo
        from d3build.target.configheader import ConfigHeader

        name = self.name
        if build.config.platform == 'linux':
            name = name.replace(' ', '_')

        args = [
            ('log.level.root', 'debug'),
            ('path.data', self.datapath),
            ('path.user', 'user'),
        ]
        if build.config.platform == 'windows':
            args.append(('log.winconsole', 'yes'))
        args = ['-d{}={}'.format(*arg) for arg in args]

        mod = SourceModule(
            sources=self.sources,
            configure=self._module_configure)
        mod = build.get_module(mod)
        build.target.add_generated_source(
            ConfigHeader(_base(__file__, '../../include/config.h'), build))
        build.target.add_generated_source(
            VersionInfo(
                _base(__file__, '../../src/core/version_const.c'),
                _base(build.script, '.'),
                _base(__file__, '../..'),
                'git'))

        if build.config.platform == 'osx':
            target = build.target.add_application_bundle(
                name=name,
                module=mod,
                info_plist=None)
        else:
            target = build.target.add_executable(
                name=name,
                module=mod,
                uuid=self.uuid)

        if build.config.platform == 'linux':
            from .runscript import RunScript
            target = build.target.add_generated_source(
                RunScript(name, name, target, args))
        build.target.add_default(target)
