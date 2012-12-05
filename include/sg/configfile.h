/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_CONFIGFILE_H
#define SG_CONFIGFILE_H
#include "libpce/hashtable.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A configuration file is a collection of variables.  Conceptually,
   each variable is identified by a qualified name and has a value
   which is a string.  The periods in a qualified name separate the
   section name from the variable name.  For example, A.B.C is a
   variable named C in section A.B.  Configuration files use a variant
   of INI file format, which is not really standardized.  */

/* Entries in a config file are sections allocated with malloc.  The
   name of each section is also allocated with malloc and is shared
   with the section structure.  */
struct configfile {
    struct pce_hashtable sect;
};

/* Entries in a config file section have each key and value set to
   nul-terminated strings allocated with malloc.  These are all owned
   by the section.  */
struct configfile_section {
    char *name;
    struct pce_hashtable var;
};

/* Initialize an empty config file.  */
void
configfile_init(struct configfile *f);

/* Destroy a config file.  */
void
configfile_destroy(struct configfile *f);

/* Get a variable from a config file, or return NULL if the variable
   does not exist.  */
const char *
configfile_get(struct configfile *f, const char *section,
               const char *name);

/* Set a variable in a config file and return 0, or return ENOMEM if
   out of memory..  */
int
configfile_set(struct configfile *f, const char *section,
               const char *name, const char *value);

/* Erase a variable from a configuration file.  If the variable is the
   only variable in its section, delete the section.  */
void
configfile_remove(struct configfile *f,
                  const char *section, const char *name);

/* ===== Low-level functions ===== */

/* Get a section in a config file, creating it if necessary.  Return
   NULL if out of memory.  */
struct configfile_section *
configfile_insert_section(struct configfile *f, const char *name);

/* Erase the given section from the configuration file.  */
void
configfile_erase_section(struct configfile *f,
                         struct configfile_section *s);

/* Set a variable in a config file section and return 0, or return
   ENOMEM if out of memory.  */
int
configfile_insert_var(struct configfile_section *s,
                      const char *name, const char *value);

/* Erase the given variable from the config file section.  */
void
configfile_erase_var(struct configfile_section *s,
                     struct pce_hashtable_entry *e);

/* ===== I/O functions ===== */

/* Read a configuration file.  Return 0 if successful or if the file
   does not exist, an error code if an error occured, or -1 if error
   in the file syntax caused the operation to abort.  Duplicate
   variables will be discarded with a warning, the first copy of each
   variable will be kept.  */
int
configfile_read(struct configfile *f, const char *path);

/* Write a configuration file and return 0, or return an error
   code.  */
int
configfile_write(struct configfile *f, const char *path);

#ifdef __cplusplus
}
#endif
#endif
