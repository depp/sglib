/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_CLOCK_H
#define SG_CLOCK_H
#ifdef __cplusplus
extern "C" {
#endif

/* The minimum buffer size for sg_clock_getdate.  */
#define SG_DATE_LEN 25

/* Return the number of seconds since the timer was initialized.  */
double
sg_clock_get(void);

/* Get the current UTC date and time as an ISO-8601 string.  The
   number of characters written is returned, not counting the nul
   terminator.  */
int
sg_clock_getdate(char *date, int shortfmt);

#ifdef __cplusplus
}
#endif
#endif
