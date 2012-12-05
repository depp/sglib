#ifndef BASE_RAND_H
#define BASE_RAND_H
#ifdef __cplusplus
extern "C" {
#endif

/* A random number generator.  */
struct sg_rand_state {
    unsigned x0, x1, c;
};

/* Global random number generator, initialized at boot with some
   platform-specific source of entropy.  */
extern struct sg_rand_state sg_rand_global;

/* Seed an array of random number generators using a platform-specific
   source of entropy.  */
void
sg_rand_seed(struct sg_rand_state *array, unsigned count);

/* Generate a pseudorandom integer in the range 0..UINT_MAX.  */
unsigned
sg_irand(struct sg_rand_state *s);

/* Global version of sg_irand.  */
unsigned
sg_girand(void);

/* Generate a pseudorandom float in the interval [0,1).  */
float
sg_frand(struct sg_rand_state *s);

/* Global version of sg_frand.  */
float
sg_gfrand(void);

#ifdef __cplusplus
}
#endif
#endif
