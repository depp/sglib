/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
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
