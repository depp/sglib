/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_CLOCK_H
#define SG_CLOCK_H
#ifdef __cplusplus
extern "C" {
#endif

/* The minimum buffer size for sg_clock_getdate.  */
#define SG_DATE_LEN 25

/* Initialize the timer, setting the current time to zero.  */
void
sg_clock_init(void);

/* Return the number of milliseconds since the timer was initialized.
   This will wrap every 2^32 ms, a little less than 50 days.  */
unsigned
sg_clock_get(void);

/* Get the current UTC date and time as an ISO-8601 string.  The
   number of characters written is returned, not counting the nul
   terminator.  */
int
sg_clock_getdate(char *date);

/* Sleep for the given number of milliseconds.  */
void
sg_clock_sleep(unsigned milliseconds);

#ifdef __cplusplus
}
#endif
#endif
