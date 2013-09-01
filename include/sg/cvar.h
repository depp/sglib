/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_CVAR_H
#define SG_CVAR_H
#ifdef __cplusplus
extern "C" {
#endif
/* Global configuration variables.  Variables are organized by section
   and by name.  When getting a variable's value, priority goes first
   to a value set by the user during this session, then to the command
   line, then to the config files.  Defaults are expected to be
   provided by another mechanism.  */

/* Set a variable from a command line parameter.  First, if name is
   NULL then it is parsed from v which must be of the form
   'name=value'.  If section is NULL, it is parsed from name which
   must be of the form 'section.name'.  */
void
sg_cvar_addarg(const char *section, const char *name, const char *v);

/* Load cvars from the configuration file.  This should be called
   after the command line parameters are parsed.  */
void
sg_cvar_load(void);

/* General getters and setters.  Each operates on a type: s=string,
   i=int, b=boolean, d=double.  The configuration file uses strings
   internally and converts to and from the desired data type.  A
   getter returns 0 if the variable is not found or if the conversion
   fails, and returns 1 if successful.  */

void
sg_cvar_sets(const char *section, const char *name, const char *v);

int
sg_cvar_gets(const char *section, const char *name, const char **v);

void
sg_cvar_seti(const char *section, const char *name, int v);

int
sg_cvar_geti(const char *section, const char *name, int *v);

void
sg_cvar_setl(const char *section, const char *name, long v);

int
sg_cvar_getl(const char *section, const char *name, long *v);

void
sg_cvar_setb(const char *section, const char *name, int v);

int
sg_cvar_getb(const char *section, const char *name, int *v);

void
sg_cvar_setd(const char *section, const char *name, double v);

int
sg_cvar_getd(const char *section, const char *name, double *v);

#ifdef __cplusplus
}
#endif
#endif
