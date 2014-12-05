/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_CLOCK_H
#define SG_CLOCK_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sg/clock.h
 *
 * @brief Clocks and timers.
 */

/**
 * @brief The minimum buffer size for sg_clock_getdate().
 */
#define SG_DATE_LEN 25

/**
 * @brief Get the number of seconds since the application started.
 *
 * Note that this value may be ahead of the value passed to
 * sg_game_draw(), because the video recording system will alter the
 * timestamps for sg_game_draw() if the video encoder is slower than
 * realtime.
 *
 * @return The number of seconds since the application started.
 */
double
sg_clock_get(void);

/**
 * @brief Get the current UTC date and time as an ISO-8601 string.
 *
 * @param date A buffer of size ::SG_DATE_LEN to store the result.
 * @param shortfmt Whether to use the short format (whole seconds).
 * @return The number of characters written, not counting the NUL
 * terminator.
 */
int
sg_clock_getdate(
    char *date,
    int shortfmt);

/**
 * @brief Timer callback.
 *
 * @param time The current time.
 * @param cxt The callback parameter.
 */
typedef void
(*sg_timer_func_t)(
    double time,
    void *cxt);

/**
 * @brief Flags for timers.
 */
enum {
    /**
     * @brief The timestamp is an absolute time.
     */
    SG_TIMER_ABSTIME = 01,

    /**
     * @brief The timestamp is relative to the current time.
     */
    SG_TIMER_RELTIME = 02,

    /**
     * @brief If the same callback and context object is registered
     * twice, keep the earlier one.
     */
    SG_TIMER_KEEP_FIRST = 04,

    /**
     * @brief If the same callback and context object is registered
     * twice, keep the later one.
     */
    SG_TIMER_KEEP_LAST = 010
};

/**
 * @brief Register a timer callback.
 *
 * Either ::SG_TIMER_ABSTIME or ::SG_TIMER_RELTIME must be specified
 * in the flags (but not both).
 *
 * @param time The time at which to execute the callback.
 * @param flags Timer flags.
 * @param callback The callback function.
 * @param cxt A parameter to pass to the callback function.
 */
void
sg_timer_register(
    double time,
    unsigned flags,
    sg_timer_func_t callback,
    void *cxt);

#ifdef __cplusplus
}
#endif
#endif
