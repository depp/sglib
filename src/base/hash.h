#ifndef BASE_HASH_H
#define BASE_HASH_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

unsigned
murmur_hash(void const *data, size_t len, unsigned seed);

#ifdef __cplusplus
}
#endif
#endif
