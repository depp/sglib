# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.

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
