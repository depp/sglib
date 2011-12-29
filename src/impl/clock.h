#ifndef IMPL_CLOCK_H
#define IMPL_CLOCK_H
#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the timer, setting the current time to zero.  */
void
sg_clock_init(void);

/* Return the number of milliseconds since the timer was initialized.
   This will wrap every 2^32 ms, a little less than 50 days.  */
unsigned
sg_clock_get(void);

#ifdef __cplusplus
}
#endif
#endif
