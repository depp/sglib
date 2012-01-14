#include "dict.h"
#include "hash.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

/* Random and arbitrary */
#define SEED 0x8482ff4U

static unsigned
round_up_pow2(unsigned x)
{
    size_t a = x - 1;
    a |= a >> 1;
    a |= a >> 2;
    a |= a >> 4;
    a |= a >> 8;
    a |= a >> 16;
    return a + 1;
}

/* Calculate capacity for hash with given number of values.  */
static unsigned
hash_margin(unsigned x)
{
    return x + (x >> 1);
}

void
dict_init(struct dict *d)
{
    d->contents = NULL;
    d->size = 0;
    d->capacity = 0;
}

void
dict_destroy(struct dict *d, void (*vfree)(struct dict_entry *))
{
    struct dict_entry *es = d->contents;
    unsigned i, n = d->capacity;
    if (vfree) {
        for (i = 0; i < n; ++i)
            if (es[i].key)
                vfree(&es[i]);
    }
    free(es);
}

struct dict_entry *
dict_get(struct dict *d, char const *key)
{
    struct dict_entry *es = d->contents;
    unsigned i, pos, n = d->capacity, hash;
    size_t len = strlen(key);
    hash = murmur_hash(key, len, SEED);
    for (i = 0; i < n; ++i) {
        pos = (i + hash) & (n - 1);
        if (!es[pos].key)
            return NULL;
        if (!strcmp(key, es[pos].key))
            return &es[pos];
    }
    return NULL;
}

static void
dict_xfer(struct dict_entry *d, unsigned dsz,
          struct dict_entry *s, unsigned ssz)
{
    unsigned i, hash, j, pos;
    for (i = 0; i < dsz; ++i) {
        d[i].key = NULL;
        d[i].value = NULL;
        d[i].hash = 0;
    }
    for (i = 0; i < ssz; ++i) {
        if (!s[i].key)
            continue;
        hash = s[i].hash;
        for (j = 0; j < dsz; ++j) {
            pos = (hash + j) & (dsz - 1);
            if (d[pos].key)
                continue;
            memcpy(d + pos, s + i, sizeof(*d));
            break;
        }
        assert(j < dsz);
    }
}

struct dict_entry *
dict_insert(struct dict *d, char *key)
{
    struct dict_entry *es = d->contents, *nes;
    unsigned i, pos = 0, n = d->capacity, nn, hash;
    size_t len = strlen(key);
    hash = murmur_hash(key, len, SEED);
    for (i = 0; i < n; ++i) {
        pos = (i + hash) & (n - 1);
        if (!es[pos].key)
            break;
        if (!strcmp(key, es[pos].key))
            return &es[pos];
    }
    nn = round_up_pow2(hash_margin(d->size + 1));
    if (nn > n) {
        nes = malloc(sizeof(*nes) * nn);
        if (!nes)
            return NULL;
        dict_xfer(nes, nn, es, n);
        d->contents = nes;
        d->capacity = nn;
        free(es);
        es = nes;
        n = nn;
        for (i = 0; i < n; ++i) {
            pos = (i + hash) & (n - 1);
            if (!es[pos].key)
                break;
        }
    }
    assert(i < n);
    es[pos].key = key;
    es[pos].value = NULL;
    es[pos].hash = hash;
    d->size += 1;
    return &es[pos];
}

void
dict_erase(struct dict *d, struct dict_entry *e)
{
    struct dict_entry *es = d->contents;
    unsigned epos = e - d->contents, ppos, pos, n = d->capacity;
    assert(epos < n);
    for (pos = epos + 1; ; ++pos) {
        pos = pos & (n - 1);
        if (!es[pos].key)
            break;
        ppos = es[pos].hash;
        if (((pos - epos) & (n - 1)) > ((pos - ppos) & (n - 1)))
            continue;
        memcpy(es + epos, es + pos, sizeof(*es));
        epos = pos;
    }
    es[epos].key = NULL;
    es[epos].value = NULL;
    es[epos].hash = 0;
    d->size -= 1;
}

static int
dict_resize(struct dict *d, unsigned size, int shrink)
{
    struct dict_entry *es = d->contents, *nes;
    unsigned n = d->capacity, nn, s;
    s = size > d->size ? size : d->size;
    nn = round_up_pow2(hash_margin(s));
    if (nn == n || (!shrink && nn < n))
        return 0;
    nes = malloc(sizeof(*nes) * nn);
    if (!nes)
        return ENOMEM;
    dict_xfer(nes, nn, es, n);
    free(es);
    d->contents = nes;
    d->capacity = nn;
    return 0;
}

int
dict_compact(struct dict *d)
{
    return dict_resize(d, d->size, 1);
}

int
dict_reserve(struct dict *d, unsigned n)
{
    return dict_resize(d, n, 0);
}
