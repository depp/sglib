#include <libpce/hashtable.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
#define debug(...) printf(__VA_ARGS__)
#endif

#ifndef debug
#define debug(...)
#endif

static unsigned
rand_next(unsigned x)
{
    return x * 1664525U + 1013904223U;
}

static
void check(struct pce_hashtable *d, unsigned count,
           char **strings, long *values)
{
    unsigned i, j, dsize = 0, isize = 0;
    struct pce_hashtable_entry *es;
    /* Check each value */
    for (i = 0; i < count; ++i) {
        es = pce_hashtable_get(d, strings[i]);
        if (values[i]) {
            dsize++;
            assert(es);
            assert((long)es->value == values[i]);
        } else
            assert(!es);
    }
    /* Check number of entries */
    assert(dsize == d->size);
    es = d->contents;
    /* Check iteration */
    for (j = 0; j < d->capacity; ++j) {
        if (!es[j].key)
            continue;
        isize += 1;
        for (i = j + 1; i < d->capacity; ++i) {
            assert(es[i].key != es[j].key);
        }
    }
    /* Check iteration count */
    assert(dsize == isize);
}

static void
fail(char const *why)
{
    fprintf(stderr, "Test failed: %s\n", why);
    exit(1);
}

int
main(int argc, char **argv)
{
    char **strings = NULL, *s;
    unsigned count = 0, size = 0, l, nsize, i, x, y;
    long ov, nv, *values;
    char buf[64];
    struct pce_hashtable dict;
    unsigned gen = 1618033988;
    struct pce_hashtable_entry *e;

    (void) argc;
    (void) argv;

    while (fgets(buf, sizeof(buf), stdin)) {
        l = strlen(buf);
        if (!l)
            continue;
        if (buf[l - 1] == '\n')
            l -= 1;
        if (count >= size) {
            nsize = size ? size * 2 : 8;
            strings = realloc(strings, sizeof(*strings) * nsize);
            assert(strings != NULL);
            size = nsize;
        }
        s = malloc(l + 1);
        assert(s != NULL);
        memcpy(s, buf, l);
        s[l] = '\0';
        strings[count] = s;
        count += 1;
    }
    fprintf(stderr, "%i lines read, testing...\n", count);
    values = calloc(count, sizeof(*values));
    assert(values != NULL);
    pce_hashtable_init(&dict);
    debug("inserting...\n");
    for (i = 0; i < count * 3; ++i) {
        x = gen % 10;
        y = (gen / 10) % count;
        gen = rand_next(gen);
        s = strings[y];
        ov = values[y];
        if (x < 8) {
            nv = (gen % 100) + 1;
            gen = rand_next(gen);
            if (x == 7) {
                if (pce_hashtable_reserve(&dict, dict.size * 2 + 1))
                    fail("pce_hashtable_reserve (1)");
            }
            debug("set %u -> %li\n", y, nv);
            e = pce_hashtable_insert(&dict, s);
            if (!e)
                fail("pce_hashtable_insert");
            if (ov)
                assert((long) e->value == ov);
            else
                assert(e->value == NULL);
            e->value = (void *) nv;
        } else {
            debug("erase %u\n", y);
            nv = 0;
            e = pce_hashtable_get(&dict, s);
            if (!ov)
                assert(e == NULL);
            else {
                assert(e != NULL);
                pce_hashtable_erase(&dict, e);
            }
        }
        values[y] = nv;
        check(&dict, count, strings, values);
    }
    debug("erasing...\n");
    while (dict.size > 0) {
        x = gen % count;
        y = (gen / count) % 20;
        gen = rand_next(gen);
        for (i = 0; i < count; ++i) {
            x = (x + 1) % count;
            if ((ov = values[x]))
                break;
        }
        s = strings[x];
        assert(s != NULL);
        e = pce_hashtable_get(&dict, s);
        assert(e != NULL);
        debug("erase %u\n", x);
        pce_hashtable_erase(&dict, e);
        values[x] = 0;
        check(&dict, count, strings, values);
        if (y == 19) {
            debug("compact to %u\n", dict.size);
            if (pce_hashtable_compact(&dict))
                fail("compact");
            check(&dict, count, strings, values);
        } else if (y == 18) {
            debug("resize to %u\n", dict.size * 3 + 10);
            if (pce_hashtable_reserve(&dict, dict.size * 3 + 10))
                fail("pce_hashtable_reserve (2)");
            check(&dict, count, strings, values);
        }
    }
    pce_hashtable_destroy(&dict, NULL);
    for (i = 0; i < count; ++i)
        free(strings[i]);
    free(strings);
    free(values);
    return 0;
}
