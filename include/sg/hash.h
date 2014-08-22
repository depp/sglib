/* Copyright 2012-2013 Dietrich Epp.
   This file is part of LibPCE.  LibPCE is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef PCE_HASH_H
#define PCE_HASH_H
#include <stddef.h>
#include "sg/attribute.h"

/**
 * @file hash.h
 *
 * @brief Hash functions.
 */

/**
 * @brief Compute the hash of a byte string.
 *
 * The hash is not guaranteed to produce the same result on different
 * platforms or when using different versions of this library.  If you
 * want consistent output between platforms, use something like CRC.
 */
PCE_ATTR_PURE
unsigned
pce_hash(const void *data, size_t len);

#endif
