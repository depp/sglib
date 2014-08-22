/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_HASH_H
#define SG_HASH_H
#ifdef __cplusplus
extern "C" {
#endif
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
SG_ATTR_PURE
unsigned
sg_hash(const void *data, size_t len);

#ifdef __cplusplus
}
#endif
#endif
