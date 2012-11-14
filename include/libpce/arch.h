/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/*
  Portable compile-time CPU architecture macros.

  Definitions:

  x86: PCE_CPU_X86
  x64: PCE_CPU_X86 and PCE_CPU_X64
  PowerPC: PCE_CPU_PPC
  PowerPC 64-bit: PCE_CPU_PPC and PCE_CPU_PPC64
  ARM: PCE_CPU_ARM
*/
#ifndef PCE_ARCH_H
#define PCE_ARCH_H

#if defined(_M_X64) || defined(__x86_64__)
# define PCE_CPU_X64 1
# define PCE_CPU_X86 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386) || \
    defined(__X86__) || defined(__I86__) || defined(__INTEL__)
# define PCE_CPU_X86 1
#elif defined(__ppc64__)
# define PCE_CPU_PPC64 1
# define PCE_CPU_PPC 1
#elif defined(__ppe__) || defined(__powerpc__)
# define PCE_CPU_PPC 1
#elif defined(__arm__) || defined(__thumb__)
# define PCE_CPU_ARM 1
#endif

#endif
