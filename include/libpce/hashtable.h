/* Copyright 2009-2013 Dietrich Epp.
   This file is part of LibPCE.  LibPCE is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef PCE_HASHTABLE_H
#define PCE_HASHTABLE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file hashtable.h
 *
 * @brief Hash table
 *
 * Simple hash table with linear probing.  You are responsible for
 * managing storage for the keys.
 */

/**
 * @brief A hash table.
 */
struct pce_hashtable {
    /**
     * @brief An array of hash table entries.
     */
    struct pce_hashtable_entry *contents;

    /**
     * @brief The number of entries in the array which have data.
     */
    size_t size;

    /**
     * @brief The size of the array.
     */
    size_t capacity;
};

/**
 * @brief An entry in a hash table.
 */
struct pce_hashtable_entry {
    /**
     * @brief The key.
     *
     * Storage for the key is not managed by the hash table.  The key
     * will be @c NULL if this entry contains no data.
     */
    char *key;

    /**
     * @brief The value.
     */
    void *value;

    /**
     * @brief The hash of the key.
     */
    unsigned hash;
};

/**
 * @brief Initialize a hash table.
 */
void
pce_hashtable_init(struct pce_hashtable *d);

/**
 * @brief Destroy a hash table.
 *
 * Calls @c vfree on each used entry.  The @c vfree parameter may be
 * @c NULL.
 */
void
pce_hashtable_destroy(struct pce_hashtable *d,
                      void (*vfree)(struct pce_hashtable_entry *));

/**
 * @brief Fetch the entry for the given key.
 *
 * Returns @c NULL if the key is not found.  If an entry is returned,
 * the entry's key will be non-NULL and be equal to the search key, as
 * measured by strcmp.
 */
struct pce_hashtable_entry *
pce_hashtable_get(struct pce_hashtable *d, char const *key);

/**
 * @brief Insert a key into the hash table.
 *
 * Returns the new hash table entry, or @c NULL if out of memory.  If
 * the key is already present in the hash table, that entry will be
 * returned.  Otherwise, a new entry will be created and its key will
 * be set to the key passed to this function, and its value will be
 * set to NULL.
 *
 * If the key needs to be copied when it is inserted, you will need to
 * copy the key yourself.
 */
struct pce_hashtable_entry *
pce_hashtable_insert(struct pce_hashtable *d, char *key);

/**
 * @brief Erase the given entry. 
 */
void
pce_hashtable_erase(struct pce_hashtable *d,
                    struct pce_hashtable_entry *e);

/**
 * @brief Compact the hash table.
 *
 * Returns 0 on success or @c ENOMEM if out of memory.
 */
int
pce_hashtable_compact(struct pce_hashtable *d);

/**
 * @brief Reserve space in a hash table.
 *
 * Reserves enough space to contain the given total number of keys.
 * Returns 0 on success or @c ENOMEM if out of memory.
 */
int
pce_hashtable_reserve(struct pce_hashtable *d, size_t n);

#ifdef __cplusplus
}
#endif
#endif
