/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include <stddef.h>

/* Maximum length of CVar name or section name.  */
#define SG_CVARLEN 16

/* A table mapping fixed-length keys to pointers.  */
struct sg_cvartable {
    char (*key)[SG_CVARLEN];
    void **value;
    unsigned count;
    unsigned size;
};

/* Copy the key into a buffer of size SG_CVARLEN + 1, return 0 on
   success, nonzero if the key is invalid.  */
int
sg_cvartable_getkey(char *keybuf, const char *key, size_t len);

/* Initialize a CVar table.  */
void
sg_cvartable_init(struct sg_cvartable *t);

/* Get the value for a key in a CVar table, or return NULL if the key
   was not found.  */
void *
sg_cvartable_get(struct sg_cvartable *t, const char *key);

/* Get a pointer to the value for a key in a CVar table, creating it
   if necessary, or return NULL if out of memory.  */
void **
sg_cvartable_insert(struct sg_cvartable *t, const char *key);
