# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.source import SourceList, _base
from d3build.config import Config
from d3build.module import SourceModule
from d3build.target.configheader import ConfigHeader
from . import options
from . import module
import sys

class App(object):
    __slots__ = [
        'name', 'sources',
        'email', 'uri', 'copyright', 'identifier', 'uuid', 'defaults',
        'configure_func',
    ]

    def __init__(self, *, name, sources,
                 email=None, uri=None, copyright=None,
                 identifier=None, uuid=None, defaults=None,
                 configure=None):
        self.name = name
        self.sources = sources
        self.email = email
        self.uri = uri
        self.copyright = copyright
        self.identifier = identifier
        self.uuid = uuid
        self.defaults = defaults
        self.configure_func = configure

    def _module_configure(self, env):
        if self.configure_func is None:
            tags = {}
        else:
            tags = dict(self.configure_func(env))
        private = list(tags.get('private', []))
        private.append(module.module)
        tags['private'] = private
        return tags

    def configure(self):
        cfg = Config.configure(options=options.flags)
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
        env = cfg.environment()
        env.redirect_log(append=False)
        mod = SourceModule(
            sources=self.sources,
            configure=self._module_configure)
        env.add_generated_source(
            ConfigHeader(_base(__file__, '../../include/config.h'), env))
        if env.platform == 'osx':
            env.target_application_bundle(
                name=self.name,
                module=mod,
                info_plist=None)
        else:
            env.target_executable(
                name=self.name,
                module=mod,
                uuid=self.uuid)
        env.finalize()
