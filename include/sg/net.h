/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_NET_H
#define SG_NET_H
#if defined(_WIN32)
#include "defs.h"
#define INCL_WINSOCK_API_TYPEDEFS 1
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

struct sg_addr {
    socklen_t len;
    union {
        struct sockaddr addr;
        struct sockaddr_in in;
        struct sockaddr_in6 in6;
    } addr;
};

/* Initialize network subsystem.  Safe to call multiple times.
   Returns 1 if successful, 0 for failure.  */
int
sg_net_init(void);

/* Convert a human-readable address to a socket address.  Uses the
   same format as URIs: "host:port", "ipv4addr:port", and
   "[ipv6addr]:port" are accepted.  */
int
sg_net_getaddr(struct sg_addr *addr, const char *str,
               struct sg_error **err);

/* Get the name for the given socket address.  The caller should
   deallocate the result with free().  */
char *
sg_net_getname(struct sg_addr *addr, struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
