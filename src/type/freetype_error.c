/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "private.h"
#include "sg/error.h"

const struct sg_error_domain SG_ERROR_FREETYPE = {
    "freetype"
};

/* FreeType error code */
struct sg_fterror {
    int code;
    const char *msg;
};

#define FT_ERRORDEF(e, v, s) { v, s },
#define FT_ERROR_START_LIST {
#define FT_ERROR_END_LIST }

static const struct sg_fterror SG_FTERROR[] =
#undef __FTERRORS_H__
#include FT_ERRORS_H
    ;

void
sg_error_freetype(struct sg_error **err, int code)
{
    int i, n = sizeof(SG_FTERROR) / sizeof(*SG_FTERROR);
    for (i = 0; i < n; i++) {
        if (SG_FTERROR[i].code == code) {
            sg_error_sets(err, &SG_ERROR_FREETYPE, code, SG_FTERROR[i].msg);
            return;
        }
    }
    sg_error_setf(err, &SG_ERROR_FREETYPE, code,
                  "Unknown FreeType error %d", code);
}
