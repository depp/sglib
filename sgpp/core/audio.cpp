/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sgpp/audio.hpp"
#include "sgpp/error.hpp"
#include <cstring>

AudioFile::Ref AudioFile::file(const char *path)
{
    sg_audio_sample *ptr;
    sg_error *err = NULL;
    ptr = sg_audio_sample_file(path, std::strlen(path), &err);
    if (!ptr)
        throw error(&err);
    return Ref(ptr);
}
