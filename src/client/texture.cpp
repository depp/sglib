#include "texture.hpp"
#include "sys/error.hpp"

Texture::Ref Texture::file(const char *path)
{
    sg_texture *ptr;
    sg_error *err = NULL;
    ptr = sg_texture_new(path, &err);
    if (!ptr)
        throw error(&err);
    return Ref(ptr);
}
