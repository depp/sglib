# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..schema import Schema, SchemaBuilder

_VARIABLES = (SchemaBuilder(sep=';', bool_values=('false', 'true'))
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
).value()

class VisualStudioSchema(Schema):
    """A schema for Visual Studio build variables."""
    __slots__ = []

    def __init__(self):
        archs=['Win32', 'x64']
        configs=['Debug', 'Release']
        variants=['{}|{}'.format(c, a) for c in configs for a in archs]
        super(VisualStudioSchema, self).__init__(
            variables=_VARIABLES,
            archs=archs,
            configs=configs,
            variants=variants)

    def get_variants(self, configs=None, archs=None):
        """Get a list of variants."""
        if archs is None and configs is None:
            return self.variants
        if archs is None:
            archs = self.archs
        else:
            for arch in archs:
                if arch not in self.archs:
                    raise ValueError('unknown architecture: {!r}'
                                     .format(arch))
        if configs is None:
            configs = self.configs
        else:
            for config in configs:
                if config not in self.configs:
                    raise ValueError('unknown configuration: {!r}'
                                     .format(config))
        return ['{}|{}'.format(c, a) for c in configs for a in archs]
