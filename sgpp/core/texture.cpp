/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sgpp/error.hpp"
#include "sgpp/texture.hpp"

Texture::Ref Texture::file(const char *path)
{
    sg_texture_image *ptr;
    sg_error *err = NULL;
    ptr = sg_texture_image_new(path, &err);
    if (!ptr)
        throw error(&err);
    return Ref(ptr);
}
