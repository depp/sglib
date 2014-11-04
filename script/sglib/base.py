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

GCC_C_WARNINGS = '''
-Wall
-Wextra
-Wpointer-arith
-Wwrite-strings
-Wmissing-prototypes
-Werror=implicit-function-declaration
-Wdouble-promotion
-Winit-self
-Wstrict-prototypes
'''.split()

GCC_CXX_WARNINGS = '''
-Wall
-Wextra
-Wpointer-arith
'''.split()

def gnumake_vars(env, *, langs=('c', 'c++'), configs=None, **kw):
    """Test flags and merge them into the default variables."""
    v = env.varset(configs=configs, **kw)
    for lang in ('c', 'c++'):
        env.test_compile(SOURCE, lang, None, [v],
                         external=False, configs=configs)
    env.schema.update_varset(env.base, v)

def gnumake_warnings(build):
    """Test for supported warning flags."""
    werror = build.get_variable_bool('WERROR', False)
    base = build.target.base()

    c_werror = env.test_compile(
        SOURCE, 'c', None,
        [env.varset(CWARN=['-Werror']), {}],
        link=False, external=False)
    c_warnings = env.test_compile(
        SOURCE, 'c', c_werror,
        [env.varset(CWARN=[flag]) for flag in GCC_C_WARNINGS],
        use_all=True, link=False, external=False)

    cxx_werror = env.test_compile(
        SOURCE, 'c++', None,
        [env.varset(CXXWARN=['-Werror']), {}],
        link=False, external=False)
    cxx_warnings = env.test_compile(
        SOURCE, 'c++', cxx_werror,
        [env.varset(CXXWARN=[flag]) for flag in GCC_CXX_WARNINGS],
        use_all=True, link=False, external=False)

    if werror:
        for v in (c_werror, cxx_werror):
            env.schema.update_varset(env.base, v)
    for v in (c_warnings, cxx_warnings):
        env.schema.update_varset(env.base, v)

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
