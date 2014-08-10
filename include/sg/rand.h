/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_RAND_H
#define SG_RAND_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sg/rand.h
 *
 * @brief Random number generator.
 */

/**
 * @brief A random number generator.
 */
struct sg_rand_state {
    /** @brief Internal state. */
    unsigned x0;
    /** @brief Internal state. */
    unsigned x1;
    /** @brief Internal state. */
    unsigned c;
};

/**
 * @brief Global random number generator.
 *
 * This is initialized at boot with some platform-specific source of
 * entropy.
 */
extern struct sg_rand_state sg_rand_global;

/**
 * @brief Seed an array of random number generators using a
 * platform-specific source of entropy.
 *
 * This will always succeed, but if there are errors the entropy may
 * be poor.
 *
 * @param array The random number generators.
 * @param count The number of generators in the array.
 */
void
sg_rand_seed(struct sg_rand_state *array, unsigned count);

/**
 * @brief Generate a pseudorandom integer.
 *
 * This is designed to give 32 bits of entropy.  The output is
 * designed to be uniform.
 *
 * @param s The random number generator.
 * @return A random number in the range <tt>0..UINT_MAX</tt>.
 */
unsigned
sg_irand(struct sg_rand_state *s);

/**
 * @brief Generate a pseudorandom integer using the global generator.
 *
 * This is designed to give 32 bits of entropy.  The output is
 * designed to be uniform.
 *
 * @return A random number in the range <tt>0..UINT_MAX</tt>.
 */
unsigned
sg_girand(void);

/**
 * @brief Generate a pseudorandom floating-point number.
 *
 * This is designed to give 32 bits of entropy.  The output is
 * designed to be uniform.
 *
 * @param s The random number generator.
 * @return A random number in the half-open range 0..1.
 */
double
sg_frand(struct sg_rand_state *s);

/**
 * @brief Generate a pseudorandom floating-point number using the
 * global generator.
 *
 * This is designed to give 32 bits of entropy.  The output is
 * designed to be uniform.
 *
 * @return A random number in the half-open range 0..1.
 */
double
sg_gfrand(void);

#ifdef __cplusplus
}
#endif
#endif
