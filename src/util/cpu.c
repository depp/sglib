/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/cpu.h"

/*
  This is ORed with CPU features, to mark the CPU features as valid.
*/
#define PCE_CPU_FEATURES_SET 0x80000000u

const struct pce_cpufeature PCE_CPUFEATURES[] = {
#if defined(PCE_CPU_X86)
    { "mmx", PCE_CPUF_MMX },
    { "sse", PCE_CPUF_SSE },
    { "sse2", PCE_CPUF_SSE2 },
    { "sse3", PCE_CPUF_SSE3 },
    { "ssse3", PCE_CPUF_SSSE3 },
    { "sse4_1", PCE_CPUF_SSE4_1 },
    { "sse4_2", PCE_CPUF_SSE4_2 },
#elif defined(PCE_CPU_PPC)
    { "altivec", PCE_CPUF_ALTIVEC },
#endif
    { "", 0 }
};

#if defined(PCE_CPU_HASFEATURES)

unsigned pce_cpufeatures;

unsigned pce_setcpufeatures(unsigned features)
{
    unsigned x;
    x = (pce_getcpufeatures() & features) | PCE_CPU_FEATURES_SET;
    pce_cpufeatures = x;
    return x & ~PCE_CPU_FEATURES_SET;
}

#else

unsigned pce_setcpufeatures(unsigned features)
{
    (void) features;
    return 0;
}

#endif

/* ========================================
   OS X sysctl
   ======================================== */

#if !defined(CPUF) && defined(__APPLE__)
#define CPUF 1
#include <sys/sysctl.h>
#include <string.h>

/*
  The sysctl names are the same as the feature names we chase, with
  the exception of ssse3, whose sysctl name is supplementalsse3.
*/

unsigned pce_getcpufeatures(void)
{
    int enabled, features = 0, r, i, k;
    size_t length;
    char name[32];
    const char *fname, *pfx = "hw.optional.";

    k = strlen(pfx);
    memcpy(name, pfx, k);
    for (i = 0; PCE_CPUFEATURES[i].name[0]; ++i) {
        fname = PCE_CPUFEATURES[i].name;
#if defined(PCE_CPU_X86)
        if (PCE_CPUFEATURES[i].feature == PCE_CPUF_SSSE3)
            fname = "supplementalsse3";
#endif
        strcpy(name + k, fname);
        length = sizeof(enabled);
        r = sysctlbyname(name, &enabled, &length, NULL, 0);
        if (r == 0 && enabled != 0)
            features |= PCE_CPUFEATURES[i].feature;
    }
    return PCE_CPU_FEATURES_SET | features;
}

#endif

/* ========================================
   x86 CPUID
   ======================================== */

#if !defined(CPUF) && defined(PCE_CPU_X86)
#define CPUF 1

struct pce_cpu_idmap {
    signed char idflag;
    unsigned char cpufeature;
};

static const struct pce_cpu_idmap PCE_CPU_EDX[] = {
    { 23, PCE_CPUF_MMX },
    { 25, PCE_CPUF_SSE },
    { 26, PCE_CPUF_SSE2 },
    { -1, 0 }
};

static const struct pce_cpu_idmap PCE_CPU_ECX[] = {
    { 0, PCE_CPUF_SSE3 },
    { 9, PCE_CPUF_SSSE3 },
    { 19, PCE_CPUF_SSE4_1 },
    { 20, PCE_CPUF_SSE4_2 },
    { -1, 0 }
};

static unsigned pce_getcpufeatures_x86_1(
    unsigned reg, const struct pce_cpu_idmap *mp)
{
    int i;
    unsigned fl = 0;
    for (i = 0; mp[i].idflag >= 0; ++i) {
        if ((reg & (1u << mp[i].idflag)) != 0 && mp[i].cpufeature > 0)
            fl |= mp[i].cpufeature;
    }
    return fl;
}

static unsigned pce_getcpufeatures_x86(unsigned edx, unsigned ecx)
{
    return PCE_CPU_FEATURES_SET |
        pce_getcpufeatures_x86_1(edx, PCE_CPU_EDX) |
        pce_getcpufeatures_x86_1(ecx, PCE_CPU_ECX);
}

#if defined(__GNUC__)

unsigned pce_getcpufeatures(void)
{
    unsigned a, b, c, d;
#if defined(__i386__) && defined(__PIC__)
    /* %ebx is used by PIC, so we can't clobber it */
    __asm__(
        "xchgl\t%%ebx, %1\n\t"
        "cpuid\n\t"
        "xchgl\t%%ebx, %1"
        : "=a"(a), "=r"(b), "=c"(c), "=d"(d)
        : "0"(1));
#else
    __asm__(
        "cpuid"
        : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
        : "0"(1));
#endif
    return pce_getcpufeatures_x86(d, c);
}

#elif defined(_MSC_VER)

unsigned pce_getcpufeatures(void)
{
    int info[4];
    __cpuid(info, 1);
    return pce_getcpufeatures_x86(info[3], info[2]);
}

#else
#warning "Unknown compiler, no CPUID support"

unsigned pce_getcpufeatures(void)
{
    return PCE_CPU_FEATURES_SET;
}

#endif

#endif

/* ========================================
   Fallback
   ======================================== */

#if !defined(CPUF) && defined(PCE_CPU_HASFEATURES)
#warning "Unknown OS, cannot determine cpu features at runtime"

unsigned pce_getcpufeatures(void)
{
    return PCE_CPU_FEATURES_SET;
}

#endif
