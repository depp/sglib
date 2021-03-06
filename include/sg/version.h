/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_VERSION_H
#define SG_VERSION_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

extern const char SG_SG_VERSION[];
extern const char SG_SG_COMMIT[];
extern const char SG_APP_VERSION[];
extern const char SG_APP_COMMIT[];

void
sg_version_lib(const char *libname,
               const char *compileversion, const char *runversion);

void
sg_version_print(void);

/* Log the version number of various libraries to the given logger.
   These must be implemented by the code that uses the library.  */

/* Writes to the buffer, always with a terminating NUL.  */
void
sg_version_os(char *verbuf, size_t bufsz);

/* Get the machine ID.  Writes to the buffer, always with a
   terminating NUL.  */
void
sg_version_machineid(char *buf, size_t bufsz);

/* Here, "platform" means GTK, SDL, etc.  The operating system version
   does not need to be logged here.  */
void
sg_version_platform(void);

void
sg_version_libjpeg(void);

void
sg_version_libpng(void);

#ifdef __cplusplus
}
#endif
#endif
