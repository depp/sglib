# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..error import ConfigError
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
        # List of all variables as (varname, value).
        'variable_list',
        # Dictionary mapping {varname: value}.
        'variable_dict',
        # Set of unused variables.
        'variable_unused',
    ]

    def __init__(self, config):
        self._config = config
        self.schema = Schema()

        self.variable_list = []
        self.variable_dict = {}
        self.variable_unused = set()
        for vardef in config.variables:
            i = vardef.find('=')
            if i < 0:
                raise UserError(
                    'invalid variable syntax: {!r}'.format(vardef))
            varname = vardef[:i]
            value = vardef[i+1:]
            self.variable_list.append((varname, value))
            self.variable_dict[varname] = value
            self.variable_unused.add(varname)

    def dump(self, *, file):
        """Dump information about the environment."""
        pass

    def get_variable(self, name, default):
        """Get one of build variables."""
        value = self.variable_dict.get(name, default)
        self.variable_unused.discard(name)
        return value

    def get_variable_bool(self, name, default):
        """Get one of the build variables as a boolean."""
        value = self.variable_dict.get(name, None)
        if value is None:
            return default
        self.variable_unused.discard(name)
        uvalue = value.upper()
        if uvalue in ('YES', 'TRUE', 'ON', 1):
            return True
        if uvalue in ('NO', 'FALSE', 'OFF', 0):
            return False
        raise ConfigError('invalid value for {}: expecting boolean'
                          .format(name))
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
