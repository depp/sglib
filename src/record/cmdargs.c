/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "cmdargs.h"
#include "sg/util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SG_CMDARG_INITDAT 64
#define SG_CMDARG_INITARR 8

const char *const SG_CMDARGS_ZERO[1] = { NULL };

void
sg_cmdargs_init(struct sg_cmdargs *cmd)
{
    cmd->dat = NULL;
    cmd->datsize = 0;
    cmd->datalloc = 0;

    cmd->arg = (char **) SG_CMDARGS_ZERO;
    cmd->argsize = 0;
    cmd->argalloc = 0;
}

void
sg_cmdargs_destroy(struct sg_cmdargs *cmd)
{
    free(cmd->dat);
    if (cmd->arg != (char **) SG_CMDARGS_ZERO)
        free(cmd->arg);
}

static int
sg_cmdargs_pushmem(struct sg_cmdargs *cmd, const char *p, size_t len)
{
    unsigned nalloc;
    void *narr;

    if (len + 1 > cmd->datalloc - cmd->datsize) {
        nalloc = sg_round_up_pow2_32((unsigned) (cmd->datsize + len + 1));
        if (nalloc <= cmd->datsize || len + 1 > nalloc - cmd->datsize)
            return -1;
        narr = realloc(cmd->dat, nalloc);
        if (!narr)
            return -1;
        cmd->dat = narr;
        cmd->datalloc = nalloc;
    }

    if (cmd->argsize + 1 >= cmd->argalloc) {
        nalloc = sg_round_up_pow2_32(cmd->argsize + 2);
        if (!nalloc)
            return -1;
        if (cmd->arg == (char **) SG_CMDARGS_ZERO)
            narr = malloc(nalloc * sizeof(char *));
        else
            narr = realloc(cmd->arg, nalloc * sizeof(char *));
        if (!narr)
            return -1;
        cmd->arg = narr;
        cmd->argalloc = nalloc;
    }

    memcpy(cmd->dat + cmd->datsize, p, len);
    cmd->dat[cmd->datsize + len] = '\0';
    cmd->arg[cmd->argsize] = cmd->dat + cmd->datsize;
    cmd->arg[cmd->argsize + 1] = NULL;

    cmd->datsize += len + 1;
    cmd->argsize += 1;

    return 0;
}

int
sg_cmdargs_push1(struct sg_cmdargs *cmd, const char *p)
{
    return sg_cmdargs_pushmem(cmd, p, strlen(p));
}

int
sg_cmdargs_pushs(struct sg_cmdargs *cmd, const char *str)
{
    const char *p, *q;
    int r;

    p = str;
    while (1) {
        q = strchr(p, ' ');
        if (!q) {
            if (*p)
                return sg_cmdargs_pushmem(cmd, p, strlen(p));
            return 0;
        }
        if (p != q) {
            r = sg_cmdargs_pushmem(cmd, p, q - p);
            if (r)
                return r;
        }
        p = q + 1;
    }
}

int
sg_cmdargs_pushf(struct sg_cmdargs *cmd, const char *fmt, ...)
{
    va_list ap;
    char buf[256];
    int r;

    va_start(ap, fmt);
    r = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (r < 0 || (size_t) r >= sizeof(buf))
        return -1;

    return sg_cmdargs_pushs(cmd, buf);
}
