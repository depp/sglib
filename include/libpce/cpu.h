/* Copyright 2012-2013 Dietrich Epp.
   This file is part of LibPCE.  LibPCE is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef PCE_CPU_H
#define PCE_CPU_H
#include "libpce/arch.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cpu.h
 *
 * @brief CPU feature detection.
 *
 * For each CPU feature, there is a corresponding flag starting with
 * @c PCE_CPUF.  The set of features enabled at runtime can be
 * detected by calling @c PCE_CPU_GETFEATURES(), which returns an
 * <tt>unsigned int</tt> with all enabled features ORed together.
 *
 * Note that flags are only defined if the target architecture can
 * support them.
 */

#if defined(PCE_CPU_X86) || defined(DOXYGEN)
/** Defined if this target has any optional features */
# define PCE_CPU_HASFEATURES 1
/** x86 MMX feature */
# define PCE_CPUF_MMX       (1u << 0)
/** x86 SSE feature */
# define PCE_CPUF_SSE       (1u << 1)
/** x86 SSE 2 feature */
# define PCE_CPUF_SSE2      (1u << 2)
/** x86 SSE 3 feature */
# define PCE_CPUF_SSE3      (1u << 3)
/** x86 SSSE 3 feature */
# define PCE_CPUF_SSSE3     (1u << 4)
/** x86 SSE 4.1 feature */
# define PCE_CPUF_SSE4_1    (1u << 5)
/** x86 SSE 4.2 feature */
# define PCE_CPUF_SSE4_2    (1u << 6)
#endif

#if defined(PCE_CPU_PPC) || defined(DOXYGEN)
# define PCE_CPU_HASFEATURES 1
/** PowerPC AltiVec / VMX feature */
# define PCE_CPUF_ALTIVEC   (1u << 0)
#endif

#if defined(DOXYGEN)

/** Get the set of enabled CPU features */
#define PCE_CPU_FEATURES()

#elif defined(PCE_CPU_HASFEATURES)

extern unsigned pce_cpufeatures;

unsigned pce_getcpufeatures(void);

#define PCE_CPU_FEATURES() \
    (pce_cpufeatures ? \
     pce_cpufeatures : \
     (pce_cpufeatures = pce_getcpufeatures()))

#else

#define CPU_FEATURES() 0

#endif

/**
 * @brief Information about a CPU feature.
 */
struct pce_cpufeature {
    /**
     * @brief Lower-case version of the feature name.
     */
    char name[8];

    /**
     * @brief Feature flag.
     */
    unsigned feature;
};

/**
 * @brief Array of features that this architecture supports.
 *
 * Terminated by a zeroed entry.
 */
extern const struct pce_cpufeature PCE_CPUFEATURE[];

/**
 * @brief Set which CPU features are allowed or disallowed.
 *
 * This is primarily used for comparing the performance and
 * correctness of vector implementations and scalar implementations.
 *
 * Returns the CPU features actually enabled, which will be the
 * intersection of the set of allowed features (the argument) with the
 * set of features that the current CPU actually supports.
 */
unsigned pce_setcpufeatures(unsigned features);

#ifdef __cplusplus
}
#endif
#endif
