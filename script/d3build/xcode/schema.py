# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..schema import Schema, SchemaBuilder, VarString

# Build variables for Xcode 6.0.1
# See also: https://developer.apple.com/library/mac/documentation/DeveloperTools/Reference/XcodeBuildSettingRef/1-Build_Setting_Reference/build_setting_ref.html

_VARIABLES = (SchemaBuilder()
    .list('ARCHS')
    .list('VALID_ARCHS')
    .list('BUILD_COMPONENTS')
    .list('BUILD_VARIANTS')
    .list('PATH_PREFIXES_EXCLUDED_FROM_HEADER_DEPENDENCIES')
    .list('FRAMEWORK_SEARCH_PATHS')
    .defs('GCC_PREPROCESSOR_DEFINITIONS')
    .defs('GCC_PREPROCESSOR_DEFINITIONS_NOT_USED_IN_PRECOMPS')
    .list('HEADER_SEARCH_PATHS')
    .list('INFOPLIST_OTHER_PREPROCESSOR_FLAGS')
    .defs('INFOPLIST_PREPROCESSOR_DEFINITIONS')
    .list('OTHER_CFLAGS')
    .list('OTHER_CPLUSPLUSFLAGS')
    .list('USER_HEADER_SEARCH_PATHS')
    .list('WARNING_CFLAGS')
    .list('IBC_OTHER_FLAGS')
    .list('IBC_PLUGIN_SEARCH_PATHS')
    .list('IBC_PLUGINS')
    .list('LD_RUNPATH_SEARCH_PATHS')
    .list('LIBRARY_SEARCH_PATHS')
    .list('OTHER_LDFLAGS')
    .list('OTHER_CODE_SIGN_FLAGS')

    # Our variables
    .list('.FRAMEWORKS')
).value()

class XcodeSchema(Schema):
    """A schema for Xcode build variables."""
    __slots__ = []

    def __init__(self):
        super(XcodeSchema, self).__init__(
            variables=_VARIABLES,
            default=VarString(None),
            archs=['Native'],
            configs=['Debug', 'Release'],
            variants=['Debug', 'Release'],
        )

    def get_variants(self, configs=None, archs=None):
        """Get a list of variants."""
        if archs is not None:
            raise ValueError(
                'this target does not support per-arch variables')
        if configs is None:
            return self.variants
        configs = list(configs)
        for config in configs:
            if config not in self.configs:
                raise ValueError(
                    'unknown configuration: {!r}'.format(config))
        return configs
