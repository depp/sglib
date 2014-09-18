/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */

/* Parameters for video encoding.  */
struct sg_videoparam {
    int rate_numer;
    int rate_denom;
    const char *command;
    const char *arguments;
    const char *extension;
};

/* Get the parameters for video encoding.  */
void
sg_videoparam_get(struct sg_videoparam *p);
