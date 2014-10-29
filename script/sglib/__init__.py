# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.source import SourceList, _base
# Note: SourceList is for export
from d3build.build import Build
from d3build.module import SourceModule
from . import options
from . import module
from .icon import Icon
from .path import _path
# Icon is for export
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
        # Category in the Apple application store.
        'apple_category',
        # The application icon, an Icon instance.
        'icon',
    ]

    def __init__(self, *, name, sources, datapath,
                 email=None, uri=None, copyright=None,
                 identifier=None, uuid=None, defaults=None,
                 configure=None, apple_category='public.app-category.games',
                 icon=None):
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
        self.apple_category = apple_category
        self.icon = icon

    def run(self):
        """Run the configuration script."""
        Build.run(
            name=self.name,
            build=self._build,
            sources=self.sources.sources + module.module.sources,
            options=options.flags,
            adjust_config=self._adjust_config,
        )

    def _adjust_config(self, cfg):
        """Adjust the configuration."""
        options.adjust_config(cfg, self.defaults)

    def _build(self, build):
        """Create the project targets."""
        from .version import VersionInfo
        from d3build.generatedsource.configheader import ConfigHeader
        build.env.library_search_path = _path('lib')

        name = self.name
        if build.config.platform == 'linux':
            name = name.replace(' ', '_')

        args = [
            ('log.level.root', 'debug'),
            ('path.data',
             os.path.join(build.target.run_srcroot, self.datapath)),
            ('path.user',
             os.path.join(build.target.run_srcroot, 'user')),
        ]
        if build.config.platform == 'windows':
            args.append(('log.winconsole', 'yes'))
        args = ['{}={}'.format(*arg) for arg in args]

        icon = self.icon.module(build) if self.icon is not None else None

        def module_configure(build):
            """Get the configuration for the main application module."""
            if self.configure_func is None:
                sources = []
                tags = {}
            else:
                sources, tags = self.configure_func(build)
                sources = list(sources)
                tags = dict(tags)
            private = list(tags.get('private', []))
            private.append(module.module)
            if icon is not None:
                private.append(icon)
            tags['private'] = private
            return sources, tags

        mod = SourceModule(
            sources=self.sources,
            configure=module_configure)
        mod = build.get_module(mod)
        build.target.add_generated_source(
            ConfigHeader(_path('include/config.h'), build))
        build.target.add_generated_source(
            VersionInfo(
                _path('src/core/version_const.c'),
                _base(build.script, '.'),
                _path('.'),
                'git'))

        if build.config.platform == 'osx':
            from d3build.generatedsource.template import TemplateFile
            main_nib = build.target.add_generated_source(
                TemplateFile(
                    _path('resources/MainMenu.xib'),
                    _path('src/core/osx/MainMenu.xib'),
                    {'EXE_NAME': self.name}))
            from .infoplist import InfoPropertyList
            info_plist = build.target.add_generated_source(
                InfoPropertyList(
                    _path('resources/Info.plist'),
                    self,
                    main_nib,
                    icon.icon if icon is not None else None))
            target = build.target.add_application_bundle(
                name=name,
                module=mod,
                info_plist=info_plist,
                arguments=args)
        else:
            target = build.target.add_executable(
                name=name,
                module=mod,
                uuid=self.uuid,
                arguments=args)

        if build.config.platform == 'linux':
            from .runscript import RunScript
            target = build.target.add_generated_source(
                RunScript(name, name, target, args))
        build.target.add_default(target)
