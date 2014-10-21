# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..error import ConfigError
from .variable import BuildVariables
from .schema import Schema
import io
import sys

class BaseEnvironment(object):
    """Base class for all configuration environments."""
    __slots__ = [
        # The command-line configuration parameters.
        '_config',
        # The schema for build variables.
        # This is empty by default, and subclasses should add entries.
        'schema',
    ]

    def __init__(self, config):
        self._config = config
        self.schema = Schema()

    def dump(self, *, file):
        """Dump information about the environment."""
        pass

    def varset(self, **kw):
        """Create a set of variables."""
        return BuildVariables(self.schema, **kw)

    def varset_parse(self, vardict, *, strict):
        """Parse a set of variables."""
        return BuildVariables.parse(self.schema, vardict, strict=strict)

    def varset_merge(self, varsets):
        """Merge sets of variables."""
        return BuildVariables.merge(self.schema, varsets)

    def header_paths(self, *, base, paths):
        """Create build variables that include a header search path."""
        raise NotImplementedError('must be implemented by subclass')

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
