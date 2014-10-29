# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..environment import BaseEnvironment
from ..schema import Schema
from ..log import logfile
from .project import read_project
import os

SCHEMA = (Schema(sep=';', bool_values=('false', 'true'))
    .string('Config.CharacterSet')
    .string('Config.PlatformToolset')
    .bool  ('Config.UseDebugLibraries')
    .bool  ('Config.WholeProgramOptimization')

    .list  ('VC.ExcludePath')
    .list  ('VC.ExecutablePath')
    .list  ('VC.IncludePath')
    .list  ('VC.LibraryPath')
    .list  ('VC.LibraryWPath')
    .list  ('VC.ReferencePath')
    .list  ('VC.SourcePath')
    .bool  ('VC.LinkIncremental')

    .list  ('ClCompile.AdditionalIncludeDirectories')
    .defs  ('ClCompile.PreprocessorDefinitions')
    .list  ('ClCompile.DisableSpecificWarnings')
    .bool  ('ClCompile.FunctionLevelLinking')
    .bool  ('ClCompile.IntrinsicFunctions')
    .string('ClCompile.Optimization')
    .bool  ('ClCompile.SDLCheck')
    .string('ClCompile.WarningLevel')

    .list  ('Link.AdditionalDependencies')
    .list  ('Link.AdditionalLibraryDirectories')
    .bool  ('Link.EnableCOMDATFolding')
    .bool  ('Link.GenerateDebugInformation')
    .bool  ('Link.OptimizeReferences')
    .bool  ('Link.TreatLinkerWarningAsErrors')
    .string('Link.Version')

    .string('Debug.Path')

    .objectlist('.ProjectReferences')
)

BASE_CONFIG = {
    'Config.PlatformToolset': 'v120',
    'Config.CharacterSet': 'Unicode',
    'ClCompile.WarningLevel': 'Level3',
    'ClCompile.SDLCheck': True,
    'Link.GenerateDebugInformation': True,
}

DEBUG_CONFIG = {
    'Config.UseDebugLibraries': True,
    'VC.LinkIncremental': True,
    'ClCompile.Optimization': 'Disabled',
    'ClCompile.PreprocessorDefinitions': ['WIN32', '_DEBUG', '_WINDOWS'],
}

RELEASE_CONFIG = {
    'Config.WholeProgramOptimization': True,
    'Config.UseDebugLibraries': False,
    'VC.LinkIncremental': False,
    'ClCompile.Optimization': 'MaxSpeed',
    'ClCompile.FunctionLevelLinking': True,
    'ClCompile.IntrinsicFunctions': True,
    'ClCompile.PreprocessorDefinitions': ['WIN32', 'NDEBUG', '_WINDOWS'],
    'Link.GenerateDebugInformation': True,
    'Link.EnableCOMDATFolding': True,
    'Link.OptimizeReferences': True,
}

class VisualStudioEnvironment(BaseEnvironment):
    """Environment for building with Visual Studio."""
    __slots__ = [
        # List of configurations.
        'configurations',
        # List of platforms.
        'platforms',
        # Map from (configuration, platform) to build variables.
        'base_vars',
        # (configuration, platform) pairs.
        'configs',
        # Map from paths to parsed projects.
        '_project_cache',
    ]

    def __init__(self, config):
        super(VisualStudioEnvironment, self).__init__(config)
        self.schema.update_schema(SCHEMA)
        self.configurations = self.get_variable(
            'CONFIGURATIONS', 'Debug;Release').split(';')
        self.platforms = self.get_variable(
            'PLATFORMS', 'Win32;x64').split(';')
        config_vars = {c: dict(BASE_CONFIG) for c in self.configurations}
        if 'Debug' in self.configurations:
            self.schema.update(config_vars['Debug'], DEBUG_CONFIG)
        if 'Release' in self.configurations:
            self.schema.update(config_vars['Release'], RELEASE_CONFIG)
        self.base_vars = {(c, p): dict(v) for c, v in config_vars.items()
                          for p in self.platforms}
        self.configs = [(c, p) for c in self.configurations
                        for p in self.platforms]
        self._project_cache = {}

    def dump(self, *, file):
        """Dump information about the environment."""
        super(VisualStudioEnvironment, self).dump(file=file)
        file = logfile(2)
        print('Configurations:', file=file)
        print('  CONFIGURATIONS={}'
              .format(';'.join(self.configurations)),
              file=file)
        print('  PLATFORMS={}'
              .format(';'.join(self.platforms)),
              file=file)
        print('Build settings:', file=file)
        for configuration in self.configurations:
            for platform in self.platforms:
                print('  {} {}:'.format(configuration, platform), file=file)
                self.schema.dump(
                    self.base_vars[configuration,platform],
                    file=file,
                    indent='    ')

    def define(self, definition):
        """Create build variables that define a preprocessor variable."""
        return {'ClCompile.PreprocessorDefinitions': [definition]}

    def header_path(self, path, *, system=False):
        """Create build variables that include a header search path."""
        return {'ClCompile.AdditionalIncludeDirectories': [path]}

    def library(self, path):
        """Create build variables that link with a library."""
        return {'Link.AdditionalDependencies': [path]}

    def library_path(self, path):
        """Create build variables that include a library search path."""
        return {'Link.AdditionalLibraryDirectories': [path],
                'Debug.Path': [path]}

    def project_reference(self, path):
        """Create build variables that reference another project."""
        try:
            return self._project_cache[path]
        except KeyError:
            pass
        if not os.path.exists(path):
            raise ConfigError('project does not exist: {}'.format(path))
        # CONFIG MAP FIXME
        project = read_project(
            path=path,
            configs={c: c for c in self.configs})
        self._project_cache[path] = project
        return {'.ProjectReferences': [project]}
