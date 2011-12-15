#ifndef IMPL_CONFIGFILE_H
#define IMPL_CONFIGFILE_H
#include "dict.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Entries in a config file are sections allocated with malloc.  The
   name of each section is also allocated with malloc and is shared
   with the section structure.  */
struct configfile {
    struct dict sect;
};

/* Entries in a config file section have each key and value set to
   nul-terminated strings allocated with malloc.  These are all owned
   by the section.  */
struct configfile_section {
    char *name;
    struct dict var;
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
                     struct dict_entry *e);

#ifdef __cplusplus
}
#endif
#endif
