from d3build.source import SourceList
from d3build.config import Config
from . import options
from . import module
import sys

class App(object):
    __slots__ = [
        'name', 'source',
        'email', 'uri', 'copyright', 'identifier', 'uuid', 'defaults',
    ]

    def __init__(self, *, name, source,
                 email=None, uri=None, copyright=None,
                 identifier=None, uuid=None, defaults=None):
        self.name = name
        self.source = source
        self.email = email
        self.uri = uri
        self.copyright = copyright
        self.identifier = identifier
        self.uuid = uuid
        self.defaults = defaults

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
        m = module.module
        cm = m.configure(env)
        print('SOURCES', cm.sources)
        print('PUBLIC', cm.public)
        print('PRIVATE', cm.private)
        print('DEPENDENCIES', cm.dependencies)
        for err in env.errors:
            err.write(sys.stderr)
