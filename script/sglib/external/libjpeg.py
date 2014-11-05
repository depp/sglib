# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.package import ExternalPackage

# stdio.h is necessary
TEST_SOURCE = '''\
#include <stdio.h>
#include "jpeglib.h"
int main(int argc, char **argv) {
    int version;
    struct jpeg_decompress_struct cinfo;
    (void) argc;
    (void) argv;
    version = JPEG_LIB_VERSION;
    jpeg_create_decompress(&cinfo);
    return version;
}
'''

def test(build):
    mod = build.target.module()
    mod.add_library('-ljpeg')
    if not mod.test_compile(TEST_SOURCE, 'c'):
        raise ConfigError('Test program failed to compile and link.')
    return None, mod

module = ExternalPackage(
    [test],
    name='LibJPEG',
    packages={
        'deb': 'libjpeg-dev',
        'rpm': 'libjpeg-turbo',
        'gentoo': 'media-libs/libjpeg-turbo',
        'arch': 'libjpeg-turbo',
    }
)
