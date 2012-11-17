/* Copyright 2009-2012 Dietrich Epp <depp@zdome.net> */
#ifndef PCE_HASHTABLE_H
#define PCE_HASHTABLE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defmodule hashtable Hash table
 *
 * Simple hash table with linear probing.  You are responsible for
 * managing storage for the keys.
 */

/**
 * A hash table.
 */
struct pce_hashtable {
    /**
     * An array of hash table entries.
     */
    struct pce_hashtable_entry *contents;

    /**
     * The number of entries in the array which have data.
     */
    size_t size;

    /**
     * The size of the array.
     */
    size_t capacity;
};

/**
 * An entry in a hash table.
 */
struct pce_hashtable_entry {
    /**
     * The key.  Storage for the key is not managed by the hash table.
     * The key will be NULL if this entry contains no data.
     */
    char *key;

    /**
     * The value.
     */
    void *value;

    /**
     * The hash of the key.
     */
    unsigned hash;
};

/**
 * Initialize a hash table.
 */
void
pce_hashtable_init(struct pce_hashtable *d);

/**
 * Destroy a hash table, and call vfree on each used entry.  The
 * callback function may be NULL.
 */
void
pce_hashtable_destroy(struct pce_hashtable *d,
                      void (*vfree)(struct pce_hashtable_entry *));

/**
 * Fetch the entry for the given key, or return NULL not found.  If an
 * entry is returned, the entry's key will be non-NULL and be equal to
 * the search key, as measured by strcmp.
 */
struct pce_hashtable_entry *
pce_hashtable_get(struct pce_hashtable *d, char const *key);

/**
 * Insert a key into the hash table and return its entry.  Returns
 * NULL if out of memory.  If the key is already present in the hash
 * table, that entry will be returned.  Otherwise, a new entry will be
 * created and its key will be set to the key passed to this function,
 * and its value will be set to NULL.
 *
 * If the key needs to be copied when it is inserted, you will need to
 * copy the key yourself.
 */
struct pce_hashtable_entry *
pce_hashtable_insert(struct pce_hashtable *d, char *key);

/**
 * Erase the given entry. 
 */
void
pce_hashtable_erase(struct pce_hashtable *d,
                    struct pce_hashtable_entry *e);

/**
 * Compact the hash table.  Returns 0 on success or ENOMEM if out of
 * memory.
 */
int
pce_hashtable_compact(struct pce_hashtable *d);

/**
 * Reserve space in the pce_hashtableionary for the given number of
 * keys. Returns 0 on success or ENOMEM if out of memory.
 */
int
pce_hashtable_reserve(struct pce_hashtable *d, size_t n);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
