# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage
from d3build.error import ConfigError

TEST_SOURCE = '''\
#include <math.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    (void) argv;
    return (int) exp(argc);
}
'''

def not_needed(build):
    if build.config.platform in ('windows', 'osx'):
        return 'not needed', build.target.module()
    raise ConfigError('must test for math library on this platform')

def lib(build):
    mod = build.target.module()
    if mod.test_compile(TEST_SOURCE, 'c'):
        return 'not needed', mod
    mod.add_library('-lm')
    if mod.test_compile(TEST_SOURCE, 'c'):
        return '-lm', mod
    raise ConfigError('cannot find math library')

module = ExternalPackage(
    [not_needed, lib],
    name='Math library',
)
