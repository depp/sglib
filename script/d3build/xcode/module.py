# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..error import ConfigError, UserError
from ..source import TagSourceFile
from ..module import Module
import os

class XcodeModule(Module):
    """A collection of source files and build settings."""
    __slots__ = []

    def add_define(self, definition, *, configs=None, archs=None):
        """Add a preprocessor definition."""
        var = {'GCC_PREPROCESSOR_DEFINITIONS': [definition]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_header_path(self, path, *,
                        configs=None, archs=None, system=False):
        """Add a header search path.

        If system is True, then the header path will be searched for
        include statements with angle brackets.  Otherwise, the header
        path will be searched for include statements with double
        quotes.  Not all targets support the distinction.
        """
        if system:
            name = 'HEADER_SEARCH_PATHS'
        else:
            name = 'USER_HEADER_SEARCH_PATHS'
        var = {name: [path]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_library(self, path, *, configs=None, archs=None):
        """Add a library."""
        var = {'OTHER_LDFLAGS': [path]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_library_path(self, path, *, configs=None, archs=None):
        """Add a library search path."""
        var = {'LIBRARY_SEARCH_PATHS': [path]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_framework(self, *, name=None, path=None,
                      configs=None, archs=None):
        """Add a framework.

        Either the name or the path should be specified (but not both).
        """
        if configs is not None or archs is not None:
            raise ConfigError('cannot add frameworks per config or arch')
        if name is not None:
            if path is not None:
                raise ValueError('cannot specify both name and path')
            path = os.path.join(
                '/System/Library/Frameworks', name + '.framework')
        elif path is None:
            raise ValueError('must specify either name or path')
        src = TagSourceFile(path, [], 'framework')
        self.add_sources([src], {})
        dirname = os.path.dirname(path)
        if dirname != '/System/Library/Frameworks':
            var = {'FRAMEWORK_SEARCH_PATHS': [dirname]}
            self.add_variables(var)
        return self

    def add_framework_path(self, path, *, configs=None, archs=None):
        """Add a framework search path."""
        var = {'FRAMEWORK_SEARCH_PATHS': [path]}
        return self.add_variables(var, configs=configs, archs=archs)
