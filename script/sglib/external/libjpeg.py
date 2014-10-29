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

def configure(build):
    varsets = [build.env.library('-ljpeg')]
    varset = build.env.test_compile_link(
        _TEST_SOURCE, 'c', None, varsets)
    return None, [], {'public': [varset]}

module = ExternalModule(
    name='LibJPEG',
    configure=configure,
    packages={
        'deb': 'libjpeg-dev',
        'rpm': 'libjpeg-turbo',
        'gentoo': 'media-libs/libjpeg-turbo',
        'arch': 'libjpeg-turbo',
    }
)
