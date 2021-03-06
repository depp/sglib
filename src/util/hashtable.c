/* Copyright 2009-2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/hash.h"
#include "sg/hashtable.h"
#include "sg/util.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

/* Calculate capacity for hash with given number of values.  */
static size_t
sg_hashtable_margin(size_t x)
{
    return x + (x >> 1);
}

void
sg_hashtable_init(struct sg_hashtable *d)
{
    d->contents = NULL;
    d->size = 0;
    d->capacity = 0;
}

void
sg_hashtable_destroy(struct sg_hashtable *d,
                     void (*vfree)(struct sg_hashtable_entry *))
{
    struct sg_hashtable_entry *es = d->contents;
    size_t i, n = d->capacity;
    if (vfree) {
        for (i = 0; i < n; ++i)
            if (es[i].key)
                vfree(&es[i]);
    }
    free(es);
}

struct sg_hashtable_entry *
sg_hashtable_get(struct sg_hashtable *d, char const *key)
{
    struct sg_hashtable_entry *es = d->contents;
    size_t i, pos, n = d->capacity, len = strlen(key);
    unsigned hash;
    hash = sg_hash(key, len);
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
sg_hashtable_xfer(struct sg_hashtable_entry *d, size_t dsz,
                  struct sg_hashtable_entry *s, size_t ssz)
{
    size_t i, j, pos;
    unsigned hash;
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

struct sg_hashtable_entry *
sg_hashtable_insert(struct sg_hashtable *d, char *key)
{
    struct sg_hashtable_entry *es = d->contents, *nes;
    size_t i, pos = 0, n = d->capacity, len = strlen(key), nn;
    unsigned hash;
    hash = sg_hash(key, len);
    for (i = 0; i < n; ++i) {
        pos = (i + hash) & (n - 1);
        if (!es[pos].key)
            break;
        if (!strcmp(key, es[pos].key))
            return &es[pos];
    }
    nn = sg_round_up_pow2(sg_hashtable_margin(d->size + 1));
    if (nn > n) {
        nes = malloc(sizeof(*nes) * nn);
        if (!nes)
            return NULL;
        sg_hashtable_xfer(nes, nn, es, n);
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
sg_hashtable_erase(struct sg_hashtable *d,
                   struct sg_hashtable_entry *e)
{
    struct sg_hashtable_entry *es = d->contents;
    size_t epos = e - d->contents, ppos, pos, n = d->capacity;
    assert(epos < n);
    for (pos = epos + 1; pos < epos + n; ++pos) {
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
sg_hashtable_resize(struct sg_hashtable *d, size_t size, int shrink)
{
    struct sg_hashtable_entry *es = d->contents, *nes;
    size_t n = d->capacity, nn, s;
    s = size > d->size ? size : d->size;
    nn = sg_round_up_pow2(sg_hashtable_margin(s));
    if (nn == n || (!shrink && nn < n))
        return 0;
    nes = malloc(sizeof(*nes) * nn);
    if (!nes)
        return ENOMEM;
    sg_hashtable_xfer(nes, nn, es, n);
    free(es);
    d->contents = nes;
    d->capacity = nn;
    return 0;
}

int
sg_hashtable_compact(struct sg_hashtable *d)
{
    return sg_hashtable_resize(d, d->size, 1);
}

int
sg_hashtable_reserve(struct sg_hashtable *d, size_t n)
{
    return sg_hashtable_resize(d, n, 0);
}
