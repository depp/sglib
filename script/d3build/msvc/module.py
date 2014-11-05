# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..module import Module
from ..error import ConfigError
from .project import read_project
import os

class VisualStudioModule(Module):
    """A collection of source files and build settings."""
    __slots__ = []

    def add_define(self, definition, *, configs=None, archs=None):
        """Add a preprocessor definition."""
        var = {'ClCompile.PreprocessorDefinitions': [definition]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_header_path(self, path, *,
                        configs=None, archs=None, system=False):
        """Add a header search path.

        If system is True, then the header path will be searched for
        include statements with angle brackets.  Otherwise, the header
        path will be searched for include statements with double
        quotes.  Not all targets support the distinction.
        """
        var = {'ClCompile.AdditionalIncludeDirectories': [path]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_library(self, path, *, configs=None, archs=None):
        """Add a library."""
        var = {'Link.AdditionalDependencies': [path]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_library_path(self, path, *, configs=None, archs=None):
        """Add a library search path."""
        var = {'Link.AdditionalLibraryDirectories': [path],
               'Debug.Path': [path]}
        return self.add_variables(var, configs=configs, archs=archs)

    def add_vcxproj(self, path, configs=None):
        """Add a dependent Visual Studio project."""
        if not os.path.exists(path):
            raise ConfigError('project does not exist: {}'.format(path))
        if configs is None:
            configs = {config: config for config in self.schema.configs}
        src = read_project(path=path, configs=configs)
        self.sources.append(src)
        return self
