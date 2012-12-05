/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sgpp/audio.hpp"
#include "sgpp/error.hpp"

AudioFile::Ref AudioFile::file(const char *path)
{
    sg_audio_file *ptr;
    sg_error *err = NULL;
    ptr = sg_audio_file_new(path, &err);
    if (!ptr)
        throw error(&err);
    return Ref(ptr);
}
