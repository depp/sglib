#ifndef IMPL_DICT_H
#define IMPL_DICT_H
#ifdef __cplusplus
extern "C" {
#endif
/* Simple dictionary implemented using a hash table with linear
   probing.  You are responsible for managing storage for the
   dictionary keys.  */

struct dict {
    struct dict_entry *contents;
    unsigned size;
    unsigned capacity;
};

struct dict_entry {
    char const *key;
    void *value;
    unsigned hash;
};

/* Initialize an empty dictionary.  */
void
dict_init(struct dict *d);

/* Destroy a dictionary, and call vfree on each used entry.  The
   callback function may be NULL.  */
void
dict_destroy(struct dict *d, void (*vfree)(struct dict_entry *));

/* Fetch the entry for the given key, or return NULL not found.  */
struct dict_entry *
dict_get(struct dict *d, char const *key);

/* Insert a key into the dictionary and return its entry.  Returns
   NULL if out of memory.  Note that the key may be already present,
   in which case, result->key != key, unless you insert the same key
   pointer twice.  Be warned.  */
struct dict_entry *
dict_insert(struct dict *d, char *key);

/* Erase the given entry.  */
void
dict_erase(struct dict *d, struct dict_entry *e);

/* Compact the dictionary.  Returns 0 on success or ENOMEM if out of
   memory.  */
int
dict_compact(struct dict *d);

/* Reserve space in the dictionary for the given number of keys.
   Returns 0 on success or ENOMEM if out of memory.  */
int
dict_reserve(struct dict *d, unsigned n);

#ifdef __cplusplus
}
#endif
#endif
