/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
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
