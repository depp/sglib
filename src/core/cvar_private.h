/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include <stddef.h>

/* Maximum length of CVar name or section name.  */
#define SG_CVAR_NAMELEN 16

/* Maximum length for cvar strings.  */
#define SG_CVAR_VALUELEN 1023

/* The default cvar section.  */
extern const char SG_CVAR_DEFAULTSECTION[SG_CVAR_NAMELEN];

/* Private cvar flags.  */
enum {
    /* The cvar has a value which should be saved to the configuration
       file in the "persistent_value" slot.  */
    SG_CVAR_HASPERSISTENT = 0100
};

/* Cvar types.  */
enum {
    SG_CVAR_STRING = 1,
    SG_CVAR_USER = 2,
    SG_CVAR_INT = 3,
    SG_CVAR_FLOAT = 4,
    SG_CVAR_BOOL = 5
};

/* All cvar sections.  */
extern struct sg_cvartable sg_cvar_section;

/* A table mapping fixed-length keys to pointers.  */
struct sg_cvartable {
    char (*key)[SG_CVAR_NAMELEN];
    void **value;
    unsigned count;
    unsigned size;
};

/* Copy the key into a buffer of size SG_CVAR_NAMELEN + 1, return 0 on
   success, nonzero if the key is invalid.  */
int
sg_cvartable_getkey(
    char *keybuf,
    const char *key,
    size_t len);

/* Initialize a CVar table.  */
void
sg_cvartable_init(
    struct sg_cvartable *t);

/* Get the value for a key in a CVar table, or return NULL if the key
   was not found.  */
void *
sg_cvartable_get(
    struct sg_cvartable *t,
    const char *key);

/* Get a pointer to the value for a key in a CVar table, creating it
   if necessary, or return NULL if out of memory.  */
void **
sg_cvartable_insert(
    struct sg_cvartable *t,
    const char *key);

/* Set a cvar.  The section and name should point to buffers of size
   SG_CVAR_NAMELEN, and the contents of these buffers should be padded
   with zeroes to fill the buffers.  The flags are the same as for
   sg_cvar_set().  */
int
sg_cvar_set2(
    const char *section,
    const char *name,
    const char *value,
    unsigned flags);
