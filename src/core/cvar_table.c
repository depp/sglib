/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "cvar_private.h"
#include "sg/hash.h"
#include "sg/util.h"
#include <stdlib.h>
#include <string.h>

static unsigned
sg_cvartable_allocsize(unsigned x)
{
    return sg_round_up_pow2_32(x + x / 2);
}

void
sg_cvartable_init(struct sg_cvartable *t) {
    t->key = NULL;
    t->value = NULL;
    t->count = 0;
    t->size = 0;
}

int
sg_cvartable_getkey(char *keybuf, const char *key, size_t len)
{
    unsigned i = 0, c;
    if (len < 1 || len > SG_CVAR_NAMELEN)
        return -1;
    if ((key[0] >= '0' && key[0] <= '9'))
        return -1;
    for (i = 0; i < len; i++) {
        c = key[i];
        if ((c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '_') {
        } else if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        } else {
            return -1;
        }
        keybuf[i] = c;
    }
    for (; i < SG_CVAR_NAMELEN + 1; i++)
        keybuf[i] = '\0';
    return 0;
}

void *
sg_cvartable_get(struct sg_cvartable *t, const char *keybuf)
{
    char (*key)[SG_CVAR_NAMELEN] = t->key;
    unsigned hash, i, pos, n = t->size;
    hash = sg_hash(keybuf, SG_CVAR_NAMELEN);
    for (i = 0; i < n; i++) {
        pos = (i + hash) & (n - 1);
        if (!key[pos][0])
            break;
        if (!memcmp(keybuf, key[pos], SG_CVAR_NAMELEN))
            return t->value[pos];
    }
    return NULL;
}

void **
sg_cvartable_insert(struct sg_cvartable *t, const char *keybuf)
{
    char (*key)[SG_CVAR_NAMELEN] = t->key, (*nkey)[SG_CVAR_NAMELEN];
    void **valuetab = t->value, **nvaluetab;
    unsigned hash, chash, i, j, pos, n = t->size, nn;
    hash = sg_hash(keybuf, SG_CVAR_NAMELEN);
    for (i = 0; i < n; i++) {
        pos = (i + hash) & (n - 1);
        if (!key[pos][0])
            break;
        if (!memcmp(keybuf, key[pos], SG_CVAR_NAMELEN))
            return &valuetab[pos];
    }
    nn = sg_cvartable_allocsize(t->count + 1);
    if (!nn)
        return NULL;
    if (nn > n) {
        nkey = calloc(nn, sizeof(*nkey));;
        if (!nkey)
            return NULL;
        nvaluetab = calloc(nn, sizeof(void *));
        if (!nvaluetab) {
            free(nkey);
            return NULL;
        }
        for (i = 0; i < n; i++) {
            if (!key[i][0])
                continue;
            chash = sg_hash(key[i], SG_CVAR_NAMELEN);
            for (j = 0; j < nn; j++) {
                pos = (chash + j) & (nn - 1);
                if (nkey[pos][0])
                    continue;
                memcpy(nkey[pos], key[i], SG_CVAR_NAMELEN);
                nvaluetab[pos] = valuetab[i];
                break;
            }
        }
        free(key);
        free(valuetab);
        t->key = key = nkey;
        t->value = valuetab = nvaluetab;
        t->size = n = nn;
        for (i = 0; i < n; i++) {
            pos = (i + hash) & (n - 1);
            if (!key[pos][0])
                break;
        }
    }
    t->count += 1;
    memcpy(key[pos], keybuf, SG_CVAR_NAMELEN);
    return &valuetab[pos];
}
