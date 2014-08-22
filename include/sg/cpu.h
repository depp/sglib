/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_CPU_H
#define SG_CPU_H
#include "sg/arch.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cpu.h
 *
 * @brief CPU feature detection.
 *
 * For each CPU feature, there is a corresponding flag starting with
 * @c SG_CPUF.  The set of features enabled at runtime can be detected
 * by calling @c SG_CPU_GETFEATURES(), which returns an <tt>unsigned
 * int</tt> with all enabled features ORed together.
 *
 * Note that flags are only defined if the target architecture can
 * support them.
 */

#if defined(SG_CPU_X86) || defined(DOXYGEN)
/** Defined if this target has any optional features */
# define SG_CPU_HASFEATURES 1
/** x86 MMX feature */
# define SG_CPUF_MMX       (1u << 0)
/** x86 SSE feature */
# define SG_CPUF_SSE       (1u << 1)
/** x86 SSE 2 feature */
# define SG_CPUF_SSE2      (1u << 2)
/** x86 SSE 3 feature */
# define SG_CPUF_SSE3      (1u << 3)
/** x86 SSSE 3 feature */
# define SG_CPUF_SSSE3     (1u << 4)
/** x86 SSE 4.1 feature */
# define SG_CPUF_SSE4_1    (1u << 5)
/** x86 SSE 4.2 feature */
# define SG_CPUF_SSE4_2    (1u << 6)
#endif

#if defined(SG_CPU_PPC) || defined(DOXYGEN)
# define SG_CPU_HASFEATURES 1
/** PowerPC AltiVec / VMX feature */
# define SG_CPUF_ALTIVEC   (1u << 0)
#endif

#if defined(DOXYGEN)

/** Get the set of enabled CPU features */
#define SG_CPU_FEATURES()

#elif defined(SG_CPU_HASFEATURES)

extern unsigned sg_cpufeatures;

unsigned sg_getcpufeatures(void);

#define SG_CPU_FEATURES() \
    (sg_cpufeatures ? \
     sg_cpufeatures : \
     (sg_cpufeatures = sg_getcpufeatures()))

#else

#define CPU_FEATURES() 0

#endif

/**
 * @brief Information about a CPU feature.
 */
struct sg_cpufeature {
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
extern const struct sg_cpufeature SG_CPUFEATURE[];

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
unsigned sg_setcpufeatures(unsigned features);

#ifdef __cplusplus
}
#endif
#endif
