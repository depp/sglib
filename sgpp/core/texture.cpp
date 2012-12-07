/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sgpp/error.hpp"
#include "sgpp/texture.hpp"
#include <cstring>

Texture::Ref Texture::file(const char *path)
{
    sg_texture *ptr;
    sg_error *err = NULL;
    ptr = sg_texture_file(path, std::strlen(path), &err);
    if (!ptr)
        throw error(&err);
    return Ref(ptr);
}
