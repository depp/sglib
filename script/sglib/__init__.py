# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.source import SourceList, _base
# Note: SourceList is for export
from d3build.build import Build
from . import options
from . import module
from . import source
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
        if isinstance(sources, SourceList):
            sources = sources.sources
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
            sources=self.sources + source.src,
            options=options.flags,
            adjust_config=self._adjust_config,
            package_search_path=_path('lib'),
        )

    def _adjust_config(self, cfg):
        """Adjust the configuration."""
        options.adjust_config(cfg, self.defaults)

    def _build(self, build):
        """Create the project targets."""
        from .version import VersionInfo
        from d3build.generatedsource.configheader import ConfigHeader

        from . import base
        base.update_base(build)

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

        mod = build.target.module()
        if self.configure_func is None:
            mod.add_sources(
                self.sources,
                {'public': [module.module(build)]})
        else:
            self.configure_func(mod)

        if self.icon is not None:
            icon, iconmod = self.icon.module(build)
            if iconmod is not None:
                mod.add_module(iconmod)
        else:
            icon = None

        mod.add_generated_source(
            ConfigHeader(_path('include/config.h'), build))
        mod.add_generated_source(
            VersionInfo(
                _path('src/core/version_const.c'),
                _base(build.script, '.'),
                _path('.'),
                'git'))

        if build.config.platform == 'osx':
            from d3build.generatedsource.template import TemplateFile
            if build.config.flags['frontend'] == 'cocoa':
                main_nib = _path('resources/MainMenu.xib')
                mod.add_generated_source(
                    TemplateFile(
                        main_nib,
                        _path('src/core/osx/MainMenu.xib'),
                        {'EXE_NAME': self.name}))
            else:
                main_nib = None
            from .infoplist import InfoPropertyList
            info_plist = _path('resources/Info.plist')
            mod.add_generated_source(
                InfoPropertyList(
                    info_plist,
                    self,
                    main_nib,
                    icon))
            build.target.add_application_bundle(
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
            default = build.get_variable('DEFAULT', 'Release')
            build.target.add_default(default)
            for variant, path in target.items():
                scriptname = '{}_{}'.format(name, variant)
                build.target.add_generated_source(
                    RunScript(scriptname, scriptname, path, args))
                build.target.add_alias(variant, [scriptname])
