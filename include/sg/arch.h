/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_ARCH_H
#define SG_ARCH_H

/**
 * @file arch.h
 *
 * @brief Architecture macros.
 */

#if defined(DOXYGEN)
/** @brief Defined for 64-bit x86 CPUs.  */
# define SG_CPU_X64 1
/** @brief Defined for x86 CPUs, including 64-bit.  */
# define SG_CPU_X86 1
/** @brief Defined for PowerPC 64-bit CPUs.  */
# define SG_CPU_PPC64 1
/** @brief Defined for PowerPC CPUs, including 64-bit.  */
# define SG_CPU_PPC 1
/** @brief Defined for ARM CPUs.  */
# define SG_CPU_ARM 1
#endif

#if defined(_M_X64) || defined(__x86_64__)
# define SG_CPU_X64 1
# define SG_CPU_X86 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386) || \
    defined(__X86__) || defined(__I86__) || defined(__INTEL__)
# define SG_CPU_X86 1
#elif defined(__ppc64__)
# define SG_CPU_PPC64 1
# define SG_CPU_PPC 1
#elif defined(__ppc__) || defined(__powerpc__)
# define SG_CPU_PPC 1
#elif defined(__arm__) || defined(__thumb__)
# define SG_CPU_ARM 1
#endif

#endif
