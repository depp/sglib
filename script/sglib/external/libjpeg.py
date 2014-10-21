# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.module import ExternalModule

# stdio.h is necessary
_TEST_SOURCE = '''\
#include <stdio.h>
#include "jpeglib.h"
int main(int argc, char **argv) {
    int version;
    struct jpeg_decompress_struct cinfo;
    version = JPEG_LIB_VERSION;
    jpeg_create_decompress(&cinfo);
    return version;
}
'''

def _configure(build):
    varsets = [build.env.varset(LIBS=('-ljpeg',))]
    varset = build.test_compile_link(
        _TEST_SOURCE, 'c', build.base_vars, varsets)
    return [], {'public': [varset]}

module = ExternalModule(
    name='LibJPEG',
    configure=_configure,
    packages={
        'deb': 'libjpeg-dev',
        'rpm': 'libjpeg-turbo',
        'gentoo': 'media-libs/libjpeg-turbo',
        'arch': 'libjpeg-turbo',
    }
)
