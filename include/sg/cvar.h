/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_CVAR_H
#define SG_CVAR_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_filedata;

/**
 * @file sg/cvar.h
 *
 * @brief Global configuration variables.
 *
 * Cvars are organized by section and by name.  They can be set by
 * engine or framework code, configured by the user, and set on the
 * command line.
 */

/**
 * @brief Public cvar flags.
 *
 * Cvars also have flags which are private to the cvar system.
 */
enum {
    /** @brief Has been modified since this flag was cleared.  */
    SG_CVAR_MODIFIED = 01,
    /** @brief Cannot be modified by the user.  */
    SG_CVAR_READONLY = 02,
    /** @brief Can only be modified with command line arguments.  */
    SG_CVAR_INITONLY = 04,
    /** @brief Saved to the user's configuration.  */
    SG_CVAR_PERSISTENT = 010,
    /* Allow a cvar to be created if it does not exist.  */
    SG_CVAR_CREATE = 020
};

/**
 * @brief Header for a cvar.
 */
struct sg_cvar_head {
    const char *doc;
    unsigned flags;
};

/**
 * @brief A string cvar.
 */
struct sg_cvar_string {
    const char *doc;
    unsigned flags;
    char *value;
    char *persistent_value;
    const char *default_value;
};

/**
 * @brief An integer cvar.
 */
struct sg_cvar_int {
    const char *doc;
    unsigned flags;
    int value;
    int persistent_value;
    int default_value;
    int min_value;
    int max_value;
};

/**
 * @brief A floating-point cvar.
 */
struct sg_cvar_float {
    const char *doc;
    unsigned flags;
    double value;
    double persistent_value;
    double default_value;
    double min_value;
    double max_value;
};

/**
 * @brief A boolean cvar.
 */
struct sg_cvar_bool {
    const char *doc;
    unsigned flags;
    int value;
    int persistent_value;
    int default_value;
};

/**
 * @brief A cvar of any type.
 */
union sg_cvar {
    struct sg_cvar_head chead;
    struct sg_cvar_string cstring;
    struct sg_cvar_int cint;
    struct sg_cvar_float cfloat;
    struct sg_cvar_bool cbool;
};

/**
 * @brief Define a string cvar.
 *
 * The default value must have static storage duration.
 *
 * @param section The cvar section.
 * @param name The cvar name.
 * @param doc The cvar description.
 * @param cvar The cvar.
 * @param value The initial value.
 * @param flags The cvar's flags.
 */
void
sg_cvar_defstring(
    const char *section,
    const char *name,
    const char *doc,
    struct sg_cvar_string *cvar,
    const char *value,
    unsigned flags);

/**
 * @brief Define an integer cvar.
 *
 * @param section The cvar section.
 * @param name The cvar name.
 * @param doc The cvar description.
 * @param cvar The cvar.
 * @param value The initial value.
 * @param min_value The minimum value.
 * @param max_value The maximum value.
 * @param flags The cvar's flags.
 */
void
sg_cvar_defint(
    const char *section,
    const char *name,
    const char *doc,
    struct sg_cvar_int *cvar,
    int value,
    int min_value,
    int max_value,
    unsigned flags);

/**
 * @brief Define a floating-point cvar.
 *
 * @param section The cvar section.
 * @param name The cvar name.
 * @param doc The cvar description.
 * @param cvar The cvar.
 * @param value The initial value.
 * @param flags The cvar's flags.
 */
void
sg_cvar_deffloat(
    const char *section,
    const char *name,
    const char *doc,
    struct sg_cvar_float *cvar,
    double value,
    double min_value,
    double max_value,
    unsigned flags);

/**
 * @brief Define a boolean cvar.
 *
 * @param section The cvar section.
 * @param name The cvar name.
 * @param doc The cvar description.
 * @param cvar The cvar.
 * @param value The initial value.
 * @param flags The cvar's flags.
 */
void
sg_cvar_defbool(
    const char *section,
    const char *name,
    const char *doc,
    struct sg_cvar_bool *cvar,
    int value,
    unsigned flags);

/**
 * @brief Set a cvar.
 *
 * The flag ::SG_CVAR_PERSISTENT indicates that the value being set
 * should be saved to the configuration file.  The flag
 * ::SG_CVAR_CREATE indicates that the cvar should be created (as a
 * string variable) if it does not exist.
 *
 * @param fullname The full name of the cvar, including the section.
 * @param fullnamelen The length of the full name.
 * @param value The new cvar value.
 * @param flags Flags affecting how the cvar will be set.
 * @return Zero if successful, nonzero if an error occurred.
 */
int
sg_cvar_set(
    const char *fullname,
    size_t fullnamelen,
    const char *value,
    unsigned flags);

/**
 * @brief Read cvars from a file.
 *
 * @param path Path to the input file.
 * @param pathlen Length of the path, in bytes.
 * @param err On failure, the error.
 */
int
sg_cvar_loadfile(
    const char *path,
    size_t pathlen,
    unsigned flags,
    struct sg_error **err);

/**
 * @brief Read cvars from a buffer.
 *
 * @param data The file buffer containing the configuration data.
 * @param err On failure, the error.
 */
int
sg_cvar_loadbuffer(
    struct sg_filedata *data,
    unsigned flags,
    struct sg_error **err);

/**
 * @brief Write all cvars to a file.
 *
 * @param path Path to the output file.
 * @param pathlen Length of the path, in bytes.
 * @param save_all Whether to save non-persistent cvars.
 * @param err On failure, the error.
 * @return Zero if successful, nonzero if an error occurred.
 */
int
sg_cvar_save(
    const char *path,
    size_t pathlen,
    int save_all,
    struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
