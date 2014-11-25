/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "log_impl.h"
#include "sg/cvar.h"
#include "sg/error.h"
#include "sg/log.h"
#include "sg/net.h"
#include "private.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define SOCKET_VALID(s) ((s) != SOCKET_ERROR)
#define NO_SOCKET SOCKET_ERROR
#define SSIZE_T int
#else
#include <unistd.h>
#define SOCKET int
#define closesocket close
#define SOCKET_VALID(s) ((s) >= 0)
#define NO_SOCKET (-1)
#define SSIZE_T ssize_t
#endif

struct sg_cvar_string sg_log_netaddr;

struct sg_log_network {
    struct sg_log_listener h;
    SOCKET sock;
    char *name;
};

#define PUT(s, l) \
    do { \
        if (sizeof(buf) - len - 4 < (l)) \
            goto overflow; \
        memcpy(buf + len, (s), (l)); \
        len += (l); \
    } while (0)

static void
sg_log_network_msg(struct sg_log_listener *llp, struct sg_log_msg *msg)
{
    struct sg_log_network *lp = (struct sg_log_network *) llp;
    char buf[512];
    size_t len = 0, pos;
    SSIZE_T r;
    SOCKET sock = lp->sock;
    struct sg_error *err = NULL;

    if (!SOCKET_VALID(sock))
        return;

    PUT(msg->time, msg->timelen);
    buf[len++] = ' ';
    PUT(msg->level, msg->levellen);
    buf[len++] = ':';
    buf[len++] = ' ';
    PUT(msg->msg, msg->msglen);
    buf[len++] = '\r';
    buf[len++] = '\n';

    pos = 0;
    while (pos < len) {
        r = send(sock, buf + pos, (int) (len - pos), 0);
        if (r < 0)
            goto error_errno;
        pos += r;
    }
    return;

overflow:
    sg_logs(SG_LOG_WARN, "log message too long");
    return;

error_errno:
#if !defined(_WIN32)
    sg_error_errno(&err, errno);
#else
    sg_error_win32(&err, WSAGetLastError());
#endif
    sg_logf(SG_LOG_ERROR, "logging to %s failed: %s", lp->name, err->msg);
    sg_error_clear(&err);
    closesocket(sock);
    lp->sock = NO_SOCKET;
    return;
}

#undef PUT

static void
sg_log_network_destroy(struct sg_log_listener *llp)
{
    struct sg_log_network *lp = (struct sg_log_network *) llp;
    if (lp->sock >= 0)
        closesocket(lp->sock);
    free(lp->name);
    free(lp);
}

void
sg_log_network_init(void)
{
    struct sg_log_network *lp = NULL;
    const char *addrstr;
    struct sg_error *err = NULL;
    struct sg_addr addr;
    char *addrname = NULL;
    int r;
    SOCKET sock = NO_SOCKET;

    sg_cvar_defstring("log", "netaddr", &sg_log_netaddr,
                      NULL, SG_CVAR_INITONLY);
    addrstr = sg_log_netaddr.value;
    if (!*addrstr)
        return;
    if (!sg_net_init())
        return;
    r = sg_net_getaddr(&addr, addrstr, &err);
    if (r) goto error;
    addrname = sg_net_getname(&addr, &err);
    if (r) goto error;
    sg_logf(SG_LOG_INFO, "connecting to %s", addrname);
#if defined SOCKET_CLOEXEC
    sock = socket(addr.addr.addr.sa_family, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
    sock = socket(addr.addr.addr.sa_family, SOCK_STREAM, 0);
#endif
    if (!SOCKET_VALID(sock)) goto error_errno;
    r = connect(sock, &addr.addr.addr, addr.len);
    if (r < 0) goto error_errno;
    lp = malloc(sizeof(*lp));
    if (!lp) goto error_nomem;
    lp->h.msg = sg_log_network_msg;
    lp->h.destroy = sg_log_network_destroy;
    lp->sock = sock;
    lp->name = addrname;
    sg_log_listen(&lp->h);
    return;

error_nomem:
    sg_error_nomem(&err);
    goto error;

error_errno:
#if !defined(_WIN32)
    sg_error_errno(&err, errno);
#else
    sg_error_win32(&err, WSAGetLastError());
#endif
    goto error;

error:
    if (SOCKET_VALID(sock))
        closesocket(sock);
    sg_logf(SG_LOG_ERROR,
            "logging to %s failed: %s",
            addrname ? addrname : addrstr, err->msg);
    sg_error_clear(&err);
    free(addrname);
}
