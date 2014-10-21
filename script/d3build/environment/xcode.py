# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .environment import BaseEnvironment
from .schema import Schema
from ..log import logfile
from ..source import _base
import re

MATCH_VAR = re.compile('^[-_A-Za-z][-_A-Za-z0-9]*$')

# Build variables for Xcode 6.0.1
# See also: https://developer.apple.com/library/mac/documentation/DeveloperTools/Reference/XcodeBuildSettingRef/1-Build_Setting_Reference/build_setting_ref.html

SCHEMA = (Schema(lax=True)
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
)

# Default configurations for new Cocoa projects, as of Xcode 6.0.1.

BASE_CONFIG = {
    'ALWAYS_SEARCH_USER_PATHS': 'NO',
    'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++0x',
    'CLANG_CXX_LIBRARY': 'libc++',
    'CLANG_ENABLE_MODULES': 'YES',
    'CLANG_ENABLE_OBJC_ARC': 'YES',
    'CLANG_WARN_BOOL_CONVERSION': 'YES',
    'CLANG_WARN_CONSTANT_CONVERSION': 'YES',
    'CLANG_WARN_DIRECT_OBJC_ISA_USAGE': 'YES_ERROR',
    'CLANG_WARN_EMPTY_BODY': 'YES',
    'CLANG_WARN_ENUM_CONVERSION': 'YES',
    'CLANG_WARN_INT_CONVERSION': 'YES',
    'CLANG_WARN_OBJC_ROOT_CLASS': 'YES_ERROR',
    'CLANG_WARN_UNREACHABLE_CODE': 'YES',
    'CLANG_WARN__DUPLICATE_METHOD_MATCH': 'YES',
    'CODE_SIGN_IDENTITY': "-",
    'ENABLE_STRICT_OBJC_MSGSEND': 'YES',
    'GCC_C_LANGUAGE_STANDARD': 'gnu99',
    'GCC_WARN_64_TO_32_BIT_CONVERSION': 'YES',
    'GCC_WARN_ABOUT_RETURN_TYPE': 'YES_ERROR',
    'GCC_WARN_UNDECLARED_SELECTOR': 'YES',
    'GCC_WARN_UNINITIALIZED_AUTOS': 'YES_AGGRESSIVE',
    'GCC_WARN_UNUSED_FUNCTION': 'YES',
    'GCC_WARN_UNUSED_VARIABLE': 'YES',
    'MACOSX_DEPLOYMENT_TARGET': '10.10',
    'SDKROOT': 'macosx',
}

DEBUG_CONFIG = {
    'COPY_PHASE_STRIP': 'NO',
    'GCC_DYNAMIC_NO_PIC': 'NO',
    'GCC_OPTIMIZATION_LEVEL': '0',
    'GCC_PREPROCESSOR_DEFINITIONS': [
        "DEBUG=1",
        "$(inherited)",
    ],
    'GCC_SYMBOLS_PRIVATE_EXTERN': 'NO',
    'MTL_ENABLE_DEBUG_INFO': 'YES',
    'ONLY_ACTIVE_ARCH': 'YES',
}

RELEASE_CONFIG = {
    'COPY_PHASE_STRIP': 'YES',
    'DEBUG_INFORMATION_FORMAT': 'dwarf-with-dsym',
    'ENABLE_NS_ASSERTIONS': 'NO',
    'MTL_ENABLE_DEBUG_INFO': 'NO',
}

class XcodeEnvironment(BaseEnvironment):
    __slots__ = [
        # List of configurations.
        'configurations',
        # Base compilation variables in this environment, per config.
        'base_vars',
    ]

    def __init__(self, config):
        super(XcodeEnvironment, self).__init__(config)
        self.schema.update_schema(SCHEMA)
        self.configurations = self.get_variable(
            'CONFIGURATIONS', 'Debug Release').split()
        base_vars = {c: dict(BASE_CONFIG) for c in self.configurations}
        if 'Debug' in self.configurations:
            base_vars['Debug'].update(DEBUG_CONFIG)
        if 'Release' in self.configurations:
            base_vars['Release'].update(RELEASE_CONFIG)

        cmap = {c.lower(): c for c in self.configurations}
        user_vars = {c: {} for c in self.configurations}
        for varname, value in self.variable_list:
            i = varname.find('.')
            if i < 0:
                continue
            cname = varname[:i].lower()
            vname = varname[i+1:]
            if not MATCH_VAR.match(vname):
                continue
            vardef = self.schema[vname]
            value = vardef.parse(value)
            if cname == 'all':
                for varset in user_vars.values():
                    varset[vname] = value
            else:
                try:
                    cname = cmap[cname]
                except KeyError:
                    continue
                varset = user_vars[cname]
                varset[vname] = value
            self.variable_used.discand(varname)
        for c in self.configurations:
            base_vars[c] = self.schema.merge([base_vars[c], user_vars[c]])

        self.base_vars = base_vars

    def dump(self, *, file):
        """Dump information about the environment."""
        super(XcodeEnvironment, self).dump(file=file)
        file = logfile(2)
        print('Configurations:', file=file)
        print('  CONFIGURATIONS={}'
              .format(' '.join(self.configurations)),
              file=file)
        print('Build settings:', file=file)
        for configuration in self.configurations:
            print('  {}:'.format(configuration), file=file)
            self.schema.dump(
                self.base_vars[configuration], file=file, indent='    ')

    def header_paths(self, *, base, paths):
        """Create build variables that include a header search path."""
        return self.schema.varset(
            USER_HEADER_SEARCH_PATHS=[_base(base, path) for path in paths])

    def frameworks(self, flist):
        """Specify a list of frameworks to use."""
        return {'.FRAMEWORKS': list(flist)}
