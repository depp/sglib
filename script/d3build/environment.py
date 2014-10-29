# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .error import ConfigError
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
        'library_search_path',
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
        self.library_search_path = None

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

    def check_platform(self, platforms):
        if isinstance(platforms, str):
            if self._config.platform == platforms:
                return
        else:
            if self._config.platform in platforms:
                return
            platforms = ', '.join(platforms)
        raise ConfigureError('only valid on platforms: {}'.format(platforms))

    def find_library(self, pattern, *, varname=None):
        if varname is not None:
            value = self.get_variable(varname, None)
            if value is not None:
                return value
        if self.library_search_path is None:
            raise ConfigError('library_path is not set')
        try:
            filenames = os.listdir(self.library_search_path)
        except FileNotFoundError:
            raise ConfigError('library path does not exist: {}'
                              .format(self.library_search_path))
        import re
        regex = re.compile(pattern)
        results = [fname for fname in filenames if regex.match(fname)]
        if not results:
            raise ConfigError('could not find library matching /{}/'
                              .format(pattern))
        if len(results) > 1:
            raise ConfigError('found multiple libraries matching /{}/: {}'
                              .format(pattern, ', '.join(results)))
        return os.path.join(self.library_search_path, results[0])

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
        """Create build variables that include a header search path.

        If system is True, then the header path will be searched for
        include statements with angle brackets.  Otherwise, the header
        path will be searched for include statements with double
        quotes.  Not all targets support the distinction.
        """
        raise NotImplementedError('must be implemented by subclass')

    def library(self, path):
        """Create build variables that link with a library."""
        raise ConfigError('library not available on this target')

    def library_path(self, path):
        """Create build variables that include a library search path."""
        raise ConfigError('library_path not available on this target')

    def framework(self, name):
        """Create build variables that link with a framework."""
        raise ConfigError('framework not available on this target')

    def framework_path(self, path):
        """Create build variables that include a framework search path."""
        raise ConfigError('framework_path not available')

    def pkg_config(self, spec):
        """Run the pkg-config tool and return the build variables."""
        raise ConfigError('pkg_config not available on this target')

    def sdl_config(self, version):
        """Run the sdl-config tool and return the build variables.

        The version should either be 1 or 2.
        """
        raise ConfigError('sdl_config not available on this target')

    def test_compile_link(self, source, sourcetype, base_varset, varsets):
        """Try different build variable sets to find one that works."""
        raise ConfigError(
            'test_compile_link not available on this target')

    def project_reference(self, path):
        """Create build variables that reference another project."""
        raise ConfigError('project_reference not available on this target')
