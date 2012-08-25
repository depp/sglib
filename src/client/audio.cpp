#include "audio.hpp"
#include "sys/error.hpp"

AudioFile::Ref AudioFile::file(const char *path)
{
    sg_audio_file *ptr;
    sg_error *err = NULL;
    ptr = sg_audio_file_new(path, &err);
    if (!ptr)
        throw error(&err);
    return Ref(ptr);
}
