#include "defs.h"

#include "error.h"
#include "net.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <Windows.h>
#endif

#define DYNAMIC_PROCS 0

#if !defined(_WIN32)

int
sg_net_init(void)
{
    return 1;
}

#else

#pragma comment(lib, "Ws2_32.lib")

#if NTDDI_VERSION < NTDDI_VISTA
typedef int (WSAAPI *inet_pton_t)(int, const char *, void *);
typedef const char *(WSAAPI *inet_ntop_t)(int, const void *, char *, size_t);
static inet_pton_t inet_pton;
static inet_ntop_t inet_ntop;

static int WSAAPI
sg_net_inet_pton(int af, const char *src, void *dest)
{
    DWORD alen;
    int r;
    union {
        struct sockaddr addr;
        struct sockaddr_in in;
        struct sockaddr_in6 in6;
    } a;

    switch (af) {
    case AF_INET:
        alen = sizeof(a.in);
        r = WSAStringToAddressA((char *) src, af, NULL, &a.addr, &alen);
        if (r) return -1;
        memcpy(dest, &a.in.sin_addr, sizeof(struct in_addr));
        return 1;

    case AF_INET6:
        alen = sizeof(a.in6);
        r = WSAStringToAddressA((char *) src, af, NULL, &a.addr, &alen);
        if (r) return -1;
        memcpy(dest, &a.in6.sin6_addr, sizeof(struct in6_addr));
        return 1;

    default:
        WSASetLastError(WSAEINVAL);
        return -1;
    }
}

static const char * WSAAPI
sg_net_inet_ntop(int af, const void *src, char *dest, size_t len)
{
    DWORD slen = (DWORD) len;
    int r;
    union {
        struct sockaddr addr;
        struct sockaddr_in in;
        struct sockaddr_in6 in6;
    } a;

    switch (af) {
    case AF_INET:
        memset(&a.in, 0, sizeof(a.in));
        a.in.sin_family = AF_INET;
        memcpy(&a.in.sin_addr, src, sizeof(struct in_addr));
        r = WSAAddressToStringA(&a.addr, sizeof(a.in), NULL, dest, &slen);
        if (r) return NULL;
        return dest;

    case AF_INET6:
        memset(&a.in6, 0, sizeof(a.in6));
        a.in6.sin6_family = AF_INET6;
        memcpy(&a.in6.sin6_addr, src, sizeof(struct in6_addr));
        r = WSAAddressToStringA(&a.addr, sizeof(a.in6), NULL, dest, &slen);
        if (r) return NULL;
        return dest;

    default:
        WSASetLastError(WSAEINVAL);
        return NULL;
    }
}

static void
sg_net_getprocs(void)
{
    HANDLE h = GetModuleHandleA("Ws2_32.dll");
    if (h) {
        inet_pton = (inet_pton_t) GetProcAddress(h, "inet_pton");
        inet_ntop = (inet_ntop_t) GetProcAddress(h, "inet_ntop");
    }
    if (!inet_pton) inet_pton = sg_net_inet_pton;
    if (!inet_ntop) inet_ntop = sg_net_inet_ntop;
}
#else
#define sg_net_getprocs(x) (void)0
#endif

int
sg_net_init(void)
{
    static int initted;
    WSADATA d;
    int r;

    if (initted)
        return initted - 1;
    r = WSAStartup(MAKEWORD(2, 2), &d);
    if (r) {
        initted = 1;
        return 0;
    }
    initted = 2;
    sg_net_getprocs();
    return 1;
}

#endif

int
sg_net_getaddr(struct sg_addr *addr, const char *str,
               struct sg_error **err)
{
    unsigned short *port;
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
        p += 1;
        host = strin;
        r = inet_pton(AF_INET6, host, &addr->addr.in6.sin6_addr);
        if (r == 0)
            goto invalid;
        if (r < 0)
            goto error_errno;
        addr->len = sizeof(struct sockaddr_in6);
        addr->addr.in6.sin6_family = AF_INET6;
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
            len = strlen(str);
            p = str + len;
            host = str;
        }
        for (i = 0; host[i]; ++i)
            if (!((host[i] >= '0' && host[i] <= '9') || host[i] == '.'))
                break;
        if (!host[i]) {
            r = inet_pton(AF_INET, host, &addr->addr.in.sin_addr);
            if (r == 0)
                goto invalid;
            if (r < 0)
                goto error_errno;
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
    *port = htons((unsigned short) v);
    return 0;

invalid:
    sg_error_sets(err, &SG_ERROR_GETADDRINFO, 0,
                  "Invalid address");
    return -1;

error_errno:
#if !defined(_WIN32)
    sg_error_errno(err, errno);
#else
    sg_error_win32(err, WSAGetLastError());
#endif
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
            if (!r)
                goto error_errno;
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
            if (!r)
                goto error_errno;
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

error_errno:
#if !defined(_WIN32)
    sg_error_errno(err, errno);
#else
    sg_error_win32(err, WSAGetLastError());
#endif
    return NULL;
}
