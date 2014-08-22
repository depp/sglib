/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/cpu.h"

/*
  This is ORed with CPU features, to mark the CPU features as valid.
*/
#define SG_CPU_FEATURES_SET 0x80000000u

const struct sg_cpufeature SG_CPUFEATURES[] = {
#if defined(SG_CPU_X86)
    { "mmx", SG_CPUF_MMX },
    { "sse", SG_CPUF_SSE },
    { "sse2", SG_CPUF_SSE2 },
    { "sse3", SG_CPUF_SSE3 },
    { "ssse3", SG_CPUF_SSSE3 },
    { "sse4_1", SG_CPUF_SSE4_1 },
    { "sse4_2", SG_CPUF_SSE4_2 },
#elif defined(SG_CPU_PPC)
    { "altivec", SG_CPUF_ALTIVEC },
#endif
    { "", 0 }
};

#if defined(SG_CPU_HASFEATURES)

unsigned sg_cpufeatures;

unsigned sg_setcpufeatures(unsigned features)
{
    unsigned x;
    x = (sg_getcpufeatures() & features) | SG_CPU_FEATURES_SET;
    sg_cpufeatures = x;
    return x & ~SG_CPU_FEATURES_SET;
}

#else

unsigned sg_setcpufeatures(unsigned features)
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

unsigned sg_getcpufeatures(void)
{
    int enabled, features = 0, r, i, k;
    size_t length;
    char name[32];
    const char *fname, *pfx = "hw.optional.";

    k = strlen(pfx);
    memcpy(name, pfx, k);
    for (i = 0; SG_CPUFEATURES[i].name[0]; ++i) {
        fname = SG_CPUFEATURES[i].name;
#if defined(SG_CPU_X86)
        if (SG_CPUFEATURES[i].feature == SG_CPUF_SSSE3)
            fname = "supplementalsse3";
#endif
        strcpy(name + k, fname);
        length = sizeof(enabled);
        r = sysctlbyname(name, &enabled, &length, NULL, 0);
        if (r == 0 && enabled != 0)
            features |= SG_CPUFEATURES[i].feature;
    }
    return SG_CPU_FEATURES_SET | features;
}

#endif

/* ========================================
   x86 CPUID
   ======================================== */

#if !defined(CPUF) && defined(SG_CPU_X86)
#define CPUF 1

struct sg_cpu_idmap {
    signed char idflag;
    unsigned char cpufeature;
};

static const struct sg_cpu_idmap SG_CPU_EDX[] = {
    { 23, SG_CPUF_MMX },
    { 25, SG_CPUF_SSE },
    { 26, SG_CPUF_SSE2 },
    { -1, 0 }
};

static const struct sg_cpu_idmap SG_CPU_ECX[] = {
    { 0, SG_CPUF_SSE3 },
    { 9, SG_CPUF_SSSE3 },
    { 19, SG_CPUF_SSE4_1 },
    { 20, SG_CPUF_SSE4_2 },
    { -1, 0 }
};

static unsigned sg_getcpufeatures_x86_1(
    unsigned reg, const struct sg_cpu_idmap *mp)
{
    int i;
    unsigned fl = 0;
    for (i = 0; mp[i].idflag >= 0; ++i) {
        if ((reg & (1u << mp[i].idflag)) != 0 && mp[i].cpufeature > 0)
            fl |= mp[i].cpufeature;
    }
    return fl;
}

static unsigned sg_getcpufeatures_x86(unsigned edx, unsigned ecx)
{
    return SG_CPU_FEATURES_SET |
        sg_getcpufeatures_x86_1(edx, SG_CPU_EDX) |
        sg_getcpufeatures_x86_1(ecx, SG_CPU_ECX);
}

#if defined(__GNUC__)

unsigned sg_getcpufeatures(void)
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
    return sg_getcpufeatures_x86(d, c);
}

#elif defined(_MSC_VER)

unsigned sg_getcpufeatures(void)
{
    int info[4];
    __cpuid(info, 1);
    return sg_getcpufeatures_x86(info[3], info[2]);
}

#else
#warning "Unknown compiler, no CPUID support"

unsigned sg_getcpufeatures(void)
{
    return SG_CPU_FEATURES_SET;
}

#endif

#endif

/* ========================================
   Fallback
   ======================================== */

#if !defined(CPUF) && defined(SG_CPU_HASFEATURES)
#warning "Unknown OS, cannot determine cpu features at runtime"

unsigned sg_getcpufeatures(void)
{
    return SG_CPU_FEATURES_SET;
}

#endif
