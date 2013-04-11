/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/*
  CPU feature detection.

  For each feature, there is a corresponding flag PCE_CPUF_feature.
  The set of features enabled at runtime can be detected by calling
  PCE_CPU_GETFEATURES(), which returns an unsigned int with all enabled
  features ORed together.

  x86 features: MMX SSE SSE2 SSE3 SSSE3 SSE4_1 SSE4_2
  PowerPC features: ALTIVEC
*/
#ifndef PCE_CPU_H
#define PCE_CPU_H
#include "libpce/arch.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(PCE_CPU_X86)
# define PCE_CPU_HASFEATURES 1
# define PCE_CPUF_MMX       (1u << 0)
# define PCE_CPUF_SSE       (1u << 1)
# define PCE_CPUF_SSE2      (1u << 2)
# define PCE_CPUF_SSE3      (1u << 3)
# define PCE_CPUF_SSSE3     (1u << 4)
# define PCE_CPUF_SSE4_1    (1u << 5)
# define PCE_CPUF_SSE4_2    (1u << 6)
#endif

#if defined(PCE_CPU_PPC)
# define PCE_CPU_HASFEATURES 1
# define PCE_CPUF_ALTIVEC   (1u << 0)
#endif

#if defined(PCE_CPU_HASFEATURES)

extern unsigned pce_cpufeatures;

unsigned pce_getcpufeatures(void);

#define PCE_CPU_FEATURES() \
    (pce_cpufeatures ? \
     pce_cpufeatures : \
     (pce_cpufeatures = pce_getcpufeatures()))

#else

#define CPU_FEATURES() 0

#endif

/*
  Information about a CPU feature.
*/
struct pce_cpufeature {
    char name[8];
    unsigned feature;
};

/*
  Array of names for the CPU features this architecture supports.
  Names are the lower case version of the feature names above, e.g.,
  PCE_CPUF_MMX becomes "mmx".  Terminated by a zeroed entry.
*/
extern const struct pce_cpufeature PCE_CPUFEATURE[];

/*
  Set which CPU features are allowed or disallowed.  This is primarily
  used for comparing the performance and correctness of vector
  implementations and scalar implementations.

  Returns the CPU features actually enabled, which will be the
  intersection of the set of allowed features (the argument) with the
  set of features that the current CPU actually supports.
*/
unsigned pce_setcpufeatures(unsigned features);

#ifdef __cplusplus
}
#endif
#endif
