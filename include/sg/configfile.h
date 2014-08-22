/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_CONFIGFILE_H
#define SG_CONFIGFILE_H
#include "sg/hashtable.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file sg/configfile.h
 *
 * @brief Configuration files.
 *
 * A configuration file is a collection of variables.  Conceptually,
 * each variable is identified by a qualified name and has a value
 * which is a string.  The periods in a qualified name separate the
 * section name from the variable name.  For example, A.B.C is a
 * variable named C in section A.B.
 */

/**
 * @brief A configuration file.
 */
struct sg_configfile {
    /**
     * @brief The configuration sections, private.
     *
     * Entries in a config file are sections allocated with malloc.
     * The name of each section is also allocated with malloc and is
     * shared with the section structure.
     */
    struct pce_hashtable sect;
};

/**
 * @brief A configuration section.
 *
 * Entries in a config file section have each key and value set to
 * nul-terminated strings allocated with malloc.  These are all owned
 * by the section.
 */
struct sg_configfile_section {
    /** @brief The section name, owned by the section.  */
    char *name;
    /** @brief The section variables, owned by the section.  */
    struct pce_hashtable var;
};

/**
 * @brief Initialize a configuration file.
 *
 * @param file The configuration file.
 */
void
sg_configfile_init(struct sg_configfile *file);

/**
 * @brief Destroy a configuration file.
 *
 * @param file The configuraiton file.
 */
void
sg_configfile_destroy(struct sg_configfile *file);

/**
 * @brief Get a variable from a configuration file.
 *
 * @param file The configuration file.
 * @param section The section name.
 * @param name The variable name.
 * @return The variable value, or NULL if it does not exist.
 */
const char *
sg_configfile_get(struct sg_configfile *file, const char *section,
                  const char *name);

/**
 * @brief Set a variable in a configuration file.
 *
 * @param file The configuration file.
 * @param section The section name.
 * @param name The variable name.
 * @param value The variable value, a NUL-terminated UTF-8 string.
 * @return Zero for success, or nonzero if out of memory.
 */
int
sg_configfile_set(struct sg_configfile *file, const char *section,
                  const char *name, const char *value);

/**
 * @brief Erase a variable from a configuration file.
 *
 * If the variable is the only variable in its section, delete the section.
 *
 * @param file The configuration file.
 * @param section The section name.
 * @param name The variable name.
 */
void
sg_configfile_remove(struct sg_configfile *file,
                     const char *section, const char *name);

/**
 * @brief Get a section from a configuration file, creating it if necessary.
 *
 * @param file The configuration file.
 * @param name The section name.
 * @return The section, or NULL if out of memory.
 */
struct sg_configfile_section *
sg_configfile_insert_section(struct sg_configfile *file, const char *name);

/**
 * @brief Erase a section from a configuration file.
 *
 * @param file The configuration file.
 * @param section The configuration section.
 */
void
sg_configfile_erase_section(struct sg_configfile *file,
                            struct sg_configfile_section *section);

/**
 * @brief Set a variable in a configuration section.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value The variable value, a NUL-terminated UTF-8 string.
 * @return Zero for success, or nonzero if out of memory.
 */
int
sg_configfile_insert_var(struct sg_configfile_section *section,
                         const char *name, const char *value);

/**
 * @brief Erase a variable from a configuration section.
 *
 * @param section The configuration section.
 * @param entry The hash bucket for the variable.
 */
void
sg_configfile_erase_var(struct sg_configfile_section *section,
                        struct pce_hashtable_entry *entry);

#ifdef __cplusplus
}
#endif
#endif
