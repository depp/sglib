#include "cvar.h"
#include "error.h"
#include "log.h"
#include "log_impl.h"
#include "net.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct sg_log_network {
    struct sg_log_listener h;
    int sock;
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
    ssize_t r;
    int sock = lp->sock;
    struct sg_error *err = NULL;

    if (sock < 0)
        return;

    PUT(msg->date, msg->datelen);
    buf[len++] = ' ';
    PUT(msg->level, msg->levellen);
    if (msg->namelen) {
        buf[len++] = ' ';
        buf[len++] = '[';
        PUT(msg->name, msg->namelen);
        buf[len++] = ']';
    }
    buf[len++] = ':';
    buf[len++] = ' ';
    PUT(msg->msg, msg->msglen);
    buf[len++] = '\r';
    buf[len++] = '\n';

    pos = 0;
    while (pos < len) {
        r = write(sock, buf + pos, len - pos);
        if (r < 0) {
            sg_error_errno(&err, errno);
            sg_logf(sg_logger_get(NULL), LOG_ERROR,
                    "logging to %s failed: %s",
                    lp->name, err->msg);
            sg_error_clear(&err);
            close(sock);
            lp->sock = -1;
            return;
        }
        pos += r;
    }
    return;

overflow:
    sg_logs(sg_logger_get(NULL), LOG_WARN, "log message too long");
}

#undef PUT

static void
sg_log_network_destroy(struct sg_log_listener *llp)
{
    struct sg_log_network *lp = (struct sg_log_network *) llp;
    if (lp->sock >= 0)
        close(lp->sock);
    free(lp->name);
    free(lp);
}

void
sg_log_network_init(void)
{
    struct sg_log_network *lp = NULL;
    struct sg_logger *logger = sg_logger_get(NULL);
    const char *addrstr;
    struct sg_error *err = NULL;
    struct sg_addr addr;
    char *addrname = NULL;
    int r;
    int sock = -1;

    r = sg_cvar_gets("log", "netaddr", &addrstr);
    if (!r) return;
    r = sg_net_getaddr(&addr, addrstr, &err);
    if (r) goto error;
    addrname = sg_net_getname(&addr, &err);
    if (r) goto error;
    sg_logf(logger, LOG_INFO, "connecting to %s", addrname);
    sock = socket(addr.addr.addr.sa_family, SOCK_STREAM, 0);
    if (sock < 0) goto error_errno;
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
    sg_error_errno(&err, errno);
    goto error;

error:
    if (sock >= 0)
        close(sock);
    sg_logf(sg_logger_get(NULL), LOG_ERROR,
            "logging to %s failed: %s",
            addrname ? addrname : addrstr, err->msg);
    sg_error_clear(&err);
    free(addrname);
    close(sock);
}
