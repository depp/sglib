/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include <libpcext/arch.h>
#include <libpcext/attribute.h>
#include <libpcext/byteorder.h>

#include <string.h>
#include <stdio.h>

#define S(x) #x
#define M(x) { #x, S(x) }
#define F(x) { "", x }

const char *const MACROS[][2] = {
    F("byteorder"),
    M(PCE_BIG_ENDIAN),
    M(PCE_LITTLE_ENDIAN),
    M(PCE_BYTE_ORDER),

    F("attribute"),
    M(PCE_ATTR_ARTIFICIAL),
    M(PCE_ATTR_CONST),
    M(PCE_ATTR_DEPRECATED),
    M(PCE_ATTR_FORMAT(a,b,c)),
    M(PCE_ATTR_MALLOC),
    M(PCE_ATTR_NOINLINE),
    M(PCE_ATTR_NONNULL((x))),
    M(PCE_ATTR_NORETURN),
    M(PCE_ATTR_NOTHROW),
    M(PCE_ATTR_PURE),
    M(PCE_ATTR_SENTINEL),
    M(PCE_ATTR_WARN_UNUSED_RESULT),

    F("cpu"),
    M(PCE_CPU_X64),
    M(PCE_CPU_X86),
    M(PCE_CPU_PPC64),
    M(PCE_CPU_PPC),
    M(PCE_CPU_ARM),

    { NULL, NULL }
};

int main(int argc, char *argv[])
{
    int i;
    for (i = 0; MACROS[i][0]; i++) {
        if (!MACROS[i][0][0])
            printf("\nlibpcext/%s.h\n", MACROS[i][1]);
        else if (!strcmp(MACROS[i][0], MACROS[i][1]))
            printf("#undef %s\n", MACROS[i][0]);
        else if (MACROS[i][1][0])
            printf("#define %s %s\n", MACROS[i][0], MACROS[i][1]);
        else
            printf("#define %s\n", MACROS[i][0]);
    }
    return 0;
}
