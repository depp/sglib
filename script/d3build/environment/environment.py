# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..error import ConfigError
from ..source import _base
from .variable import BuildVariables
from .feedback import Feedback
import io
import sys

_FORWARD = {'warnings', 'werror', 'platform', 'verbosity'}

class Tee(io.TextIOBase):
    __slots__ = ['files']
    def __init__(self, files):
        super(Tee, self).__init__()
        self.files = tuple(files)
    def writable(self):
        return True
    def write(self, s):
        for fp in self.files:
            fp.write(s)

class Flags(object):
    __slots__ = ['_vars']
    def __init__(self, config):
        self._vars = {key.replace('-', '_'): value
                      for key, value in config.flags.items()}
    def __getattr__(self, name):
        try:
            return self._vars[name]
        except KeyError:
            raise AttributeError(name)

class BaseEnvironment(object):
    """Base class for all configuration environments."""
    __slots__ = [
        '_config', 'flags',
        'modules', 'errors', 'missing_packages',
        'console', 'logfile',
        'generated_sources',
    ]

    def __init__(self, config):
        self._config = config
        self.flags = Flags(config)
        self.modules = {}
        self.errors = []
        self.missing_packages = []
        self.generated_sources = []

    def redirect_log(self, *, append):
        self.console = sys.stderr
        mode = 'a' if append else 'w'
        self.logfile = open('config.log', mode)
        sys.stderr = Tee([sys.stderr, self.logfile])

    def feedback(self, msg):
        return Feedback(self, msg)

    def __getattr__(self, name):
        if name in _FORWARD:
            return getattr(self._config, name)
        raise AttributeError(name)

    def header_path(self, base, path):
        return BuildVariables(CPPPATH=(_base(base, path),))

    def pkg_config(self, spec):
        """Run the pkg-config tool and return the build variables."""
        raise ConfigError('pkg-config not available for this target')

    def sdl_config(self, version):
        """Run the sdl-config tool and return the build variables.

        The version should either be 1 or 2.
        """
        raise ConfigError('sdl-config not available for this target')

    def frameworks(self, flist):
        """Specify a list of frameworks to use."""
        raise ConfigError('frameworks not available on this target')

    def test_compile_link(self, source, sourcetype, base_varset, varsets):
        """Try different build variable sets to find one that works."""
        raise ConfigError(
            'compile and link tests not available for this target')

    def add_generated_source(self, source):
        self.generated_sources.append(source)

    def finalize(self):
        for source in self.generated_sources:
            if source.is_regenerated_only:
                continue
            with open(source.target, 'w') as fp:
                source.write(fp)
