/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"

/* Command-line arguments for a program.  */
struct sg_cmdargs {
    char *dat;
    unsigned datsize;
    unsigned datalloc;

    char **arg;
    unsigned argsize;
    unsigned argalloc;
};

/* Initialize command-line arguments.  */
void
sg_cmdargs_init(struct sg_cmdargs *cmd);

/* Destroy command-line arguments.  */
void
sg_cmdargs_destroy(struct sg_cmdargs *cmd);

/* Push a single command-line argument.  */
int
sg_cmdargs_push1(struct sg_cmdargs *cmd, const char *p);

/* Push a list of command-line arguments, separated by spaces.  */
int
sg_cmdargs_pushs(struct sg_cmdargs *cmd, const char *str);

/* Push a list of command-line arguments, separated by spaces, after
   performing formatting.  Note that spaces inside format arguments
   will cause the format arguments to be split across multiple
   command-line arguments.  */
SG_ATTR_FORMAT(printf, 2, 3)
int
sg_cmdargs_pushf(struct sg_cmdargs *cmd, const char *fmt, ...);
