/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/file.h"
#include <stdlib.h>

static void
sg_buffer_free_(struct sg_buffer *fbuf)
{
    if (fbuf->data)
        free(fbuf->data);
    free(fbuf);
}

void
sg_buffer_incref(struct sg_buffer *fbuf)
{
    sg_atomic_inc(&fbuf->refcount);
}

void
sg_buffer_decref(struct sg_buffer *fbuf)
{
    int c = sg_atomic_fetch_add(&fbuf->refcount, -1);
    if (c == 1)
        sg_buffer_free_(fbuf);
}
