# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.log import logfile, Feedback
from d3build.error import ConfigError

SOURCE = '''\
int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    return 0;
}
'''

WARNING_FLAGS = [('c', 'CWARN', '''
-Wall
-Wextra
-Wpointer-arith
-Wwrite-strings
-Wmissing-prototypes
-Werror=implicit-function-declaration
-Wdouble-promotion
-Winit-self
-Wstrict-prototypes
'''.split()),
('c++', 'CXXWARN', '''
-Wall
-Wextra
-Wpointer-arith
'''.split())]

def gnumake_vars(build, *, langs=('c', 'c++'), configs=None, **kw):
    """Test flags and merge them into the default variables."""
    mod = build.target.module().add_variables(kw, configs=configs)
    valid = all(
        mod.test_compile(SOURCE, sourcetype, configs=configs)
        for sourcetype in langs)
    if valid:
        build.target.base.add_variables(kw, configs=configs)

def gnumake_warnings(build):
    """Test for supported warning flags."""
    want_werror = build.get_variable_bool('WERROR', False)
    for sourcetype, varname, wflags in WARNING_FLAGS:
        mod = build.target.module().add_variables({varname: ['-Werror']})
        has_werror = mod.test_compile(
            SOURCE, sourcetype, link=False, external=False)
        for wflag in wflags:
            flags = ['-Werror', wflag] if has_werror else [wflag]
            mod = build.target.module().add_variables({varname: flags})
            valid = mod.test_compile(
                SOURCE, sourcetype, link=False, external=False)
            if valid:
                build.target.base.add_variables({varname: [wflag]})
        if want_werror and has_werror:
            build.target.base.add_variables({varname: ['-Werror']})

SANITIZERS = ['address', 'leak', 'thread', 'undefined']

def gnumake_environment(build):
    """Update the GnuMake environment with the standard settings."""

    with Feedback('Checking for threading support...') as fb:
        value = ['-pthread']
        gnumake_vars(build, CFLAGS=value, CXXFLAGS=value, LDFLAGS=value)

    with Feedback('Checking for C++11 support...') as fb:
        gnumake_vars(build, CXXFLAGS=['-std=gnu++11'], langs=('c++',))

    if build.get_variable_bool('WARNINGS', True):
        with Feedback('Checking for warning flags...') as fb:
            gnumake_warnings(build)

    if build.get_variable_bool('LTO', True):
        try:
            with Feedback('Checking for link-time optimization...') as fb:
                gnumake_vars(
                    build,
                    CFLAGS=['-g0', '-flto'],
                    CXXFLAGS=['-g0', '-flto'],
                    LDFLAGS=['-flto'],
                    configs=['Release'])
        except ConfigError:
            pass

    for sanitizer in SANITIZERS:
        varname = 'SANITIZE_' + sanitizer.upper()
        if not build.get_variable_bool(varname, False):
            continue
        with Feedback('Checking for {} sanitizer...'.format(sanitizer)) as fp:
            value = ['-fsanitize=' + sanitizer]
            gnumake_vars(build, CFLAGS=value, CXXFLAGS=value, LDFLAGS=value)

    log = logfile(2)
    print('Base build variables:', file=log)
    build.target.base.variables().dump(file=log, indent='  ')

def update_base(build):
    if build.config.target == 'gnumake':
        gnumake_environment(build)
