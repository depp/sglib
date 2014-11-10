/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
struct sg_error;

extern const struct sg_error_domain SG_ERROR_FREETYPE;

void
sg_error_freetype(struct sg_error **err, int code);
