# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..error import ConfigError
from .schema import Schema
import io
import os
import sys

class BaseEnvironment(object):
    """Base class for all configuration environments."""
    __slots__ = [
        # The command-line configuration parameters.
        '_config',
        # The schema for build variables.
        # This is empty by default, and subclasses should add entries.
        'schema',
        # Path to bundled libraries.
        'library_path',
        # Dictionary mapping {varname: value}.
        'variables',
        # List of all variables as (varname, value).
        'variable_list',
        # Set of unused variables.
        'variable_unused',
    ]

    def __init__(self, config):
        self._config = config
        self.schema = Schema()
        self.library_path = None

        self.variables = {}
        self.variable_list = []
        self.variable_unused = set()
        for vardef in config.variables:
            i = vardef.find('=')
            if i < 0:
                raise UserError(
                    'invalid variable syntax: {!r}'.format(vardef))
            varname = vardef[:i]
            value = vardef[i+1:]
            self.variables[varname] = value
            self.variable_list.append((varname, value))
            self.variable_unused.add(varname)

    def dump(self, *, file):
        """Dump information about the environment."""
        pass

    def get_variable(self, name, default):
        """Get one of build variables."""
        value = self.variables.get(name, default)
        self.variable_unused.discard(name)
        return value

    def get_variable_bool(self, name, default):
        """Get one of the build variables as a boolean."""
        value = self.variables.get(name, None)
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

    def find_library(self, pattern):
        if self.library_path is None:
            raise ConfigError('library_path is not set')
        try:
            filenames = os.listdir(self.library_path)
        except FileNotFoundError:
            raise ConfigError('library path does not exist: {}'
                              .format(self.library_path))
        import re
        regex = re.compile(pattern)
        results = [fname for fname in filenames if regex.match(fname)]
        if not results:
            raise ConfigError('could not find library matching /{}/'
                              .format(pattern))
        if len(results) > 1:
            raise ConfigError('found multiple libraries matching /{}/: {}'
                              .format(pattern, ', '.join(results)))
        return os.path.join(self.library_path, results[0])

    def find_framework(self, name):
        if self._config.platform != 'osx':
            raise ConfigError('frameworks not available on this platform')
        try:
            home = os.environ['HOME']
        except KeyError:
            raise ConfigError('missing HOME environment variable')
        dir_paths = [
            os.path.join(home, 'Library/Frameworks'),
            '/Library/Frameworks',
            '/System/Library/Frameworks']
        framework_name = name + '.framework'
        for dir_path in dir_paths:
            try:
                fnames = os.listdir(dir_path)
            except FileNotFoundError:
                continue
            if framework_name in fnames:
                return os.path.join(dir_path, framework_name)
        raise ConfigError('could not find framework {}'
                          .format(framework_name))

    def define(self, definition):
        """Create build variables that define a preprocessor variable."""
        raise NotImplementedError('must be implemented by subclass')

    def header_path(self, path, *, system=False):
        """Create build variables that include a header search path."""
        raise NotImplementedError('must be implemented by subclass')

    def framework_path(self, path):
        """Create build variables that include a framework search path."""
        raise NotImplementedError('framework_path not available')

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
