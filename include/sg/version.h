/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_VERSION_H
#define SG_VERSION_H
#ifdef __cplusplus
extern "C" {
#endif
struct sg_logger;

extern const char SG_SG_VERSION[];
extern const char SG_SG_COMMIT[];
extern const char SG_APP_VERSION[];
extern const char SG_APP_COMMIT[];

void
sg_version_lib(struct sg_logger *lp, const char *libname,
               const char *compileversion, const char *runversion);

void
sg_version_print(void);

/* Log the version number of various libraries to the given logger.
   These must be implemented by the code that uses the library.  */

/* Here, "platform" means GTK, SDL, etc.  The operating system version
   does not need to be logged here.  */
void
sg_version_platform(struct sg_logger *lp);

void
sg_version_libjpeg(struct sg_logger *lp);

void
sg_version_libpng(struct sg_logger *lp);

void
sg_version_pango(struct sg_logger *lp);

#ifdef __cplusplus
}
#endif
#endif
