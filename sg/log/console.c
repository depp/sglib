/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "impl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char sg_log_prevdate[10];

static void
sg_log_console_msg(struct sg_log_listener *h, struct sg_log_msg *m)
{
    const char *date = m->date;
    size_t datelen = m->datelen;
    if (datelen >= 10 && (date[10] == 'T' || date[10] == ' ')) {
        if (memcmp(date, sg_log_prevdate, 10)) {
            memcpy(sg_log_prevdate, date, 10);
        } else {
            date += 11;
            datelen -= 11;
        }
    }
    
    (void) h;
    
    fwrite(date, 1, datelen, stderr);
    putc(' ', stderr);
    fwrite(m->level, 1, m->levellen, stderr);
    fputs(": ", stderr);
    fwrite(m->msg, 1, m->msglen, stderr);
    putc('\n', stderr);
}

static void
sg_log_console_destroy(struct sg_log_listener *h)
{
    (void) h;
}

/* Create a console and attach stdin, stdout, and stderr to the
   console.  Return 1 if a exists or was created, return 0 if no
   console exists.  Consoles only need to be created on Windows.  On
   Linux and Mac OS X we assume that the standard I/O streams work at
   launch.  */
static int
sg_log_console_create(void);

void
sg_log_console_init(void)
{
    struct sg_log_listener *l;
    if (!sg_log_console_create())
        return;
    l = malloc(sizeof(*l));
    if (!l)
        abort();
    l->msg = sg_log_console_msg;
    l->destroy = sg_log_console_destroy;
    sg_log_listen(l);
    sg_log_console_create();
}

#if defined(_WIN32)
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include "sg/cvar.h"

#define BUFFER_SIZE BUFSIZ

static void
sg_log_console_reopen(FILE *fp, DWORD which, const char *mode)
{
    FILE *f;
    char *buf = NULL;
    int fdes;
    HANDLE h;
    h = GetStdHandle(which);
    fdes = _open_osfhandle((intptr_t) h, _O_TEXT);
    /*
    buf = malloc(BUFFER_SIZE);
    if (!buf)
        abort();
    */
    f = _fdopen(fdes, mode);
    setvbuf(f, buf, _IONBF, 1);
    *fp = *f;
}

static int
sg_log_console_create(void)
{
    BOOL br;
    int flag;
    if (!sg_cvar_getb("log", "winconsole", &flag) || !flag)
        return 0;
    br = AllocConsole();
    if (!br)
        return 0;
    sg_log_console_reopen(stdin, STD_INPUT_HANDLE, "r");
    sg_log_console_reopen(stdout, STD_OUTPUT_HANDLE, "w");
    sg_log_console_reopen(stderr, STD_ERROR_HANDLE, "w");
    return 1;
}

#else

static int
sg_log_console_create(void)
{
    return 1;
}

#endif
