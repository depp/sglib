/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef PCE_HASH_H
#define PCE_HASH_H
#include <stddef.h>
#include <libpce/attribute.h>

/**
 * Compute the hash of a byte string.  The hash is not guaranteed to
 * produce the same result on different platforms.  If you want
 * consistent output between platforms, use something like CRC.
 */
PCE_ATTR_PURE
unsigned
pce_hash(const void *data, size_t len);

#endif
