#include "impl/cvar.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *CFG[][3] = {
    { "section", "name", "value" },
    { "section", "name2", "value2" },
    { "section2", "int", "123" },
    { "section2", "hex", "0x123f" },
    { "section2", "neg", "-12345" },
    { "section2", "bool1", "false" },
    { "section2", "bool2", "TRUE" },
    { "section2", "bool3", "On" },
    { "section2", "bool4", "oFF" },
    { "section2", "bool5", "Yes" },
    { "section2", "bool6", "no" },
    { "section2", "dbl1", "123.5" },
    { "section2", "dbl2", "-1e10" },
    { "section2", "inf1", "+inf" },
    { "section2", "inf2", "-inf" },
    { "section2", "inf3", "inf" },
    { "section2", "nan1", "nan" }
};

static void testi(const char *s, const char *n, long v)
{
    long u;
    int r;
    r = sg_cvar_getl(s, n, &u);
    if (!r) {
        printf("get %s.%s failed\n", s, n);
        exit(1);
    }
    if (u == v)
        return;
    printf("testi %s.%s failed\n", s, n);
    exit(1);
}

static void testb(const char *s, const char *n, int v)
{
    int u = -1;
    int r;
    r = sg_cvar_getb(s, n, &u);
    if (!r) {
        printf("get %s.%s failed\n", s, n);
        exit(1);
    }
    if (u == v)
        return;
    printf("testb %s.%s failed\n", s, n);
    exit(1);
}

static void testd(const char *s, const char *n, double v)
{
    double u = -1.0;
    int r;
    r = sg_cvar_getd(s, n, &u);
    if (!r) {
        printf("get %s.%s failed\n", s, n);
        exit(1);
    }
    if (isnan(v) && isnan(u))
        return;
    if (v == u)
        return;
    printf("testd %s.%s failed\n", s, n);
    exit(1);
}

int main(int argc, char *argv[])
{
    const char *sn = "A_section2";
    int i, j, ntest, r;
    char buf[256], buf2[256];
    const char *sv;
    (void) argc;
    (void) argv;
    ntest = sizeof(CFG) / sizeof(*CFG);
    for (i = 0; i < ntest; ++i) {
        strcpy(buf, "A_");
        strcat(buf, CFG[i][0]);
        sg_cvar_addarg(buf, CFG[i][1], CFG[i][2]);
    }
    for (i = 0; i < ntest; ++i) {
        strcpy(buf, "B_");
        strcat(buf, CFG[i][0]);
        strcat(buf, ".");
        strcat(buf, CFG[i][1]);
        sg_cvar_addarg(0, buf, CFG[i][2]);
    }
    for (i = 0; i < ntest; ++i) {
        strcpy(buf, "C_");
        strcat(buf, CFG[i][0]);
        strcpy(buf2, CFG[i][1]);
        strcat(buf2, "=");
        strcat(buf2, CFG[i][2]);
        sg_cvar_addarg(buf, 0, buf2);
    }
    for (i = 0; i < ntest; ++i) {
        strcpy(buf, "D_");
        strcat(buf, CFG[i][0]);
        strcat(buf, ".");
        strcat(buf, CFG[i][1]);
        strcat(buf, "=");
        strcat(buf, CFG[i][2]);
        sg_cvar_addarg(0, 0, buf);
    }
    for (i = 0; i < 512; ++i) {
        snprintf(buf, sizeof(buf), "section.subsection.var%d=%d", i, i);
        sg_cvar_addarg(0, 0, buf);
    }
    for (j = 'A'; j <= 'D'; ++j) {
        for (i = 0; i < ntest; ++i) {
            buf[0] = j;
            buf[1] = '_';
            buf[2] = '\0';
            strcat(buf, CFG[i][0]);
            sv = NULL;
            r = sg_cvar_gets(buf, CFG[i][1], &sv);
            if (!r) {
                printf("Fail retrieving %s.%s\n", buf, CFG[i][1]);
                return 1;
            }
            if (!sv || strcmp(sv, CFG[i][2])) {
                printf("Wrong result retrieving %s.%s\n", buf, CFG[i][1]);
                return 1;
            }
        }
    }
    testi(sn, "int", 123);
    testi(sn, "hex", 0x123f);
    testi(sn, "neg", -12345);
    testb(sn, "bool1", 0);
    testb(sn, "bool2", 1);
    testb(sn, "bool3", 1);
    testb(sn, "bool4", 0);
    testb(sn, "bool5", 1);
    testb(sn, "bool6", 0);
    testd(sn, "dbl1", 123.5);
    testd(sn, "dbl2", -1e10);
    testd(sn, "inf1", INFINITY);
    testd(sn, "inf2", -INFINITY);
    testd(sn, "inf3", INFINITY);
    testd(sn, "nan1", NAN);
    return 0;
}
