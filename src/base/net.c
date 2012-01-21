#include "error.h"
#include "net.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

int
sg_net_getaddr(struct sg_addr *addr, const char *str,
               struct sg_error **err)
{
    uint16_t *port;
    char strin[NI_MAXHOST], *end;
    const char *p, *host;
    struct addrinfo *a, *ap, hint;
    size_t len;
    int r, i;
    unsigned long v;

    memset(addr, 0, sizeof(*addr));
    if (str[0] == '[') {
        p = strchr(str + 1, ']');
        if (!p)
            goto invalid;
        len = p - (str + 1);
        if (len >= sizeof(strin))
            goto invalid;
        memcpy(strin, str + 1, len);
        strin[len] = '\0';
        r = inet_pton(AF_INET6, strin, &addr->addr.in6.sin6_addr);
        if (r == 0) {
            goto invalid;
        } else if (r < 0) {
            sg_error_errno(err, errno);
            return -1;
        }
        addr->len = sizeof(struct sockaddr_in6);
        addr->addr.in6.sin6_family = AF_INET6;
        p += 1;
        port = &addr->addr.in6.sin6_port;
    } else {
        p = strchr(str, ':');
        if (p) {
            len = p - str;
            if (len >= sizeof(strin))
                goto invalid;
            memcpy(strin, str, len);
            strin[len] = '\0';
            host = strin;
        } else {
            host = str;
        }
        for (i = 0; host[i]; ++i)
            if (!((host[i] >= '0' && host[i] <= '9') || host[i] == '.'))
                break;
        if (!host[i]) {
            r = inet_pton(AF_INET, host, &addr->addr.in.sin_addr);
            if (r == 0) {
                goto invalid;
            } else if (r < 0) {
                sg_error_errno(err, errno);
                return -1;
            }
            addr->len = sizeof(struct sockaddr_in);
            addr->addr.in.sin_family = AF_INET;
            port = &addr->addr.in.sin_port;
        } else {
            memset(&hint, 0, sizeof(hint));
            hint.ai_family = AF_UNSPEC;
            hint.ai_socktype = SOCK_STREAM;
            r = getaddrinfo(host, NULL, &hint, &a);
            if (r) {
                sg_error_gai(err, r);
                return -1;
            }
            for (ap = a; ap; ap = ap->ai_next) {
                switch (ap->ai_family) {
                case AF_INET:
                    assert(ap->ai_addrlen <= sizeof(struct sockaddr_in));
                    addr->len = sizeof(struct sockaddr_in);
                    memcpy(&addr->addr.in, ap->ai_addr, ap->ai_addrlen);
                    port = &addr->addr.in.sin_port;
                    goto haveaddr;

                case AF_INET6:
                    assert(ap->ai_addrlen <= sizeof(struct sockaddr_in6));
                    addr->len = sizeof(struct sockaddr_in6);
                    memcpy(&addr->addr.in6, ap->ai_addr, ap->ai_addrlen);
                    port = &addr->addr.in6.sin6_port;
                    goto haveaddr;

                default:
                    break;
                }
            }
        haveaddr:
            freeaddrinfo(a);
            if (!ap) {
                sg_error_gai(err, EAI_FAMILY);
                return -1;
            }
        }
    }

    if (!p || *p != ':' || !*(p + 1))
        goto invalid;
    v = strtoul(p + 1, &end, 10);
    if (v > 0xffff || *end)
        goto invalid;
    *port = htons(v);
    return 0;

invalid:
    sg_error_sets(err, &SG_ERROR_GETADDRINFO, 0,
                  "Invalid address");
    return -1;
}

static char *
sg_net_fmtint(char *p, int x)
{
    char tmp[10];
    int i = 0, j;
    do {
        tmp[i++] = '0' + (x % 10);
        x /= 10;
    } while (x);
    for (j = 0; j < i; ++j)
        p[j] = tmp[i - 1 - j];
    return p + j;
}

char *
sg_net_getname(struct sg_addr *addr, struct sg_error **err)
{
    char name[INET6_ADDRSTRLEN + 16], *namep, *p;
    const char *r;
    struct sockaddr_in *in;
    struct sockaddr_in6 *in6;
    int port;
    size_t len;
    p = name;

    switch (addr->addr.addr.sa_family) {
    case AF_INET:
        in = &addr->addr.in;
        if (in->sin_addr.s_addr != ntohl(INADDR_ANY)) {
            r = inet_ntop(AF_INET, &in->sin_addr, p, sizeof(name) - 12);
            if (!r) {
                sg_error_errno(err, errno);
                return NULL;
            }
            p += strlen(p);
        } else {
            *p++ = '*';
        }
        port = in->sin_port;
        break;

    case AF_INET6:
        in6 = &addr->addr.in6;
        if (memcmp(&in6->sin6_addr, &in6addr_any,
                   sizeof(struct in6_addr))) {
            *p++ = '[';
            r = inet_ntop(AF_INET6, &in6->sin6_addr, p, sizeof(name) - 12);
            if (!r) {
                sg_error_errno(err, errno);
                return NULL;
            }
            p += strlen(p);
            *p++ = ']';
        } else {
            *p++ = '*';
        }
        port = in6->sin6_port;
        break;
        
    default:
        sg_error_gai(err, EAI_FAMILY);
        return NULL;
    }

    *p++ = ':';
    p = sg_net_fmtint(p, ntohs(port));
    len = p - name;
    namep = malloc(len + 1);
    if (!namep) {
        sg_error_nomem(err);
        return NULL;
    }
    memcpy(namep, name, len);
    namep[len] = '\0';
    return namep;
}
