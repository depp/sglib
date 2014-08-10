/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_CVAR_H
#define SG_CVAR_H
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file sg/cvar.h
 *
 * @brief Global configuration variables.
 *
 * Variables are organized by section and by name.  When getting a
 * variable's value, priority goes first to a value set by the user
 * during this session, then to the command line, then to the config
 * files.  Defaults are expected to be provided by another mechanism.
 *
 * Configuration variables always hold strings as values, but
 * functions are provided which automatically convert values to and
 * from strings.
 */

/**
 * @brief Set a variable from a command line parameter.
 *
 * If the name is NULL then it is parsed from the value, which must be
 * of the form <tt>name=value</tt>.  If the section is NULL, then it
 * is parsed from the name, which must be of the form
 * <tt>section.name</tt>.
 *
 * @param section The configuration variable section, can be NULL.
 * @param name The configuration variable name, can be NULL.
 * @param value The configuration variable value.
 */
void
sg_cvar_addarg(const char *section, const char *name,
               const char *value);

/**
 * @brief Set a configuration variable to a string.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value The value to set.
 */
void
sg_cvar_sets(const char *section, const char *name, const char *value);

/**
 * @brief Get a configuration variable as a string.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value On return, the value if found.
 * @return 1 if the variable was found, 0 otherwise.
 */
int
sg_cvar_gets(const char *section, const char *name, const char **value);

/**
 * @brief Set a configuration variable to an integer.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value The value to set.
 */
void
sg_cvar_seti(const char *section, const char *name, int value);

/**
 * @brief Get a configuration variable as an integer.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value On return, the value if found.
 * @return 1 if the variable was found and conversion was successful,
 * 0 otherwise.
 */
int
sg_cvar_geti(const char *section, const char *name, int *value);

/**
 * @brief Set a configuration variable to a long integer.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value The value to set.
 */
void
sg_cvar_setl(const char *section, const char *name, long value);

/**
 * @brief Get a configuration variable as a long integer.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value On return, the value if found.
 * @return 1 if the variable was found and conversion was successful,
 * 0 otherwise.
 */
int
sg_cvar_getl(const char *section, const char *name, long *value);

/**
 * @brief Set a configuration variable to a boolean.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value The value to set.
 */
void
sg_cvar_setb(const char *section, const char *name, int value);

/**
 * @brief Get a configuration variable as a boolean.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value On return, the value if found.
 * @return 1 if the variable was found and conversion was successful,
 * 0 otherwise.
 */
int
sg_cvar_getb(const char *section, const char *name, int *value);

/**
 * @brief Set a configuration variable to a floating-point number.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value The value to set.
 */
void
sg_cvar_setd(const char *section, const char *name, double value);

/**
 * @brief Get a configuration variable as a floating-point number.
 *
 * @param section The configuration section.
 * @param name The variable name.
 * @param value On return, the value if found.
 * @return 1 if the variable was found and conversion was successful,
 * 0 otherwise.
 */
int
sg_cvar_getd(const char *section, const char *name, double *value);

#ifdef __cplusplus
}
#endif
#endif
