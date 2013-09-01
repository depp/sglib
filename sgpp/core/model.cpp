/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#if 0

#include "sys/error.hpp"
#include "model.hpp"
#include "color.hpp"

void Model::draw(Color tcolor, Color lcolor) const
{
    (void) &tcolor;
    (void) &lcolor;
    if (!m_ptr)
        return;
    sg_model_draw(m_ptr);
}

Model::Ref Model::file(const char *path)
{
    sg_model *ptr;
    sg_error *err = NULL;
    ptr = sg_model_new(path, &err);
    if (!ptr)
        throw error(&err);
    return Ref(ptr);
}

Model::Ref Model::mstatic(sg_model_static_t which)
{
    sg_model *ptr;
    sg_error *err = NULL;
    ptr = sg_model_static(which, &err);
    if (!ptr)
        throw error(&err);
    return Ref(ptr);
}

#endif
