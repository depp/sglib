/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sg/configfile.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

void
configfile_init(struct configfile *f)
{
    pce_hashtable_init(&f->sect);
}

static void
configfile_section_free(struct configfile_section *s)
{
    struct pce_hashtable_entry
        *p = s->var.contents, *e = p + s->var.capacity;
    for (; p != e; ++p) {
        if (p->key) {
            free(p->key);
            free(p->value);
        }
    }
    free(s->var.contents);
    free(s->name);
    free(s);
}

void
configfile_destroy(struct configfile *f)
{
    struct pce_hashtable_entry
        *p = f->sect.contents, *e = p + f->sect.capacity;
    for (; p != e; ++p) {
        if (p->key)
            configfile_section_free(p->value);
    }
    free(f->sect.contents);
}

const char *
configfile_get(struct configfile *f, const char *section,
               const char *name)
{
    struct pce_hashtable_entry *e;
    struct configfile_section *s;
    e = pce_hashtable_get(&f->sect, section);
    if (!e)
        return NULL;
    s = e->value;
    e = pce_hashtable_get(&s->var, name);
    if (!e)
        return NULL;
    return e->value;
}

int
configfile_set(struct configfile *f, const char *section,
               const char *name, const char *value)
{
    struct configfile_section *s;
    s = configfile_insert_section(f, section);
    if (!s)
        return ENOMEM;
    return configfile_insert_var(s, name, value);
}

void
configfile_remove(struct configfile *f,
                  const char *section, const char *name)
{
    struct pce_hashtable_entry *se, *ve;
    struct configfile_section *s;
    se = pce_hashtable_get(&f->sect, section);
    if (!se)
        return;
    s = se->value;
    ve = pce_hashtable_get(&s->var, name);
    if (!ve)
        return;
    configfile_erase_var(s, ve);
    if (!s->var.size) {
        configfile_section_free(s);
        pce_hashtable_erase(&f->sect, se);
    }
}

struct configfile_section *
configfile_insert_section(struct configfile *f, const char *name)
{
    struct pce_hashtable_entry *e;
    struct configfile_section *s;
    char *nname;
    size_t len;
    e = pce_hashtable_insert(&f->sect, (char *) name);
    if (!e)
        return NULL;
    if (e->value)
        return e->value;
    len = strlen(name);
    nname = malloc(len + 1);
    if (!nname) {
        pce_hashtable_erase(&f->sect, e);
        return NULL;
    }
    memcpy(nname, name, len + 1);
    s = malloc(sizeof(*s));
    if (!s) {
        pce_hashtable_erase(&f->sect, e);
        free(nname);
        return NULL;
    }
    e->key = nname;
    e->value = s;
    s->name = nname;
    pce_hashtable_init(&s->var);
    return s;
}

void
configfile_erase_section(struct configfile *f,
                         struct configfile_section *s)
{
    struct pce_hashtable_entry *e;
    e = pce_hashtable_get(&f->sect, s->name);
    if (!e)
        return;
    assert(e->value == s);
    configfile_section_free(s);
    pce_hashtable_erase(&f->sect, e);
}

int
configfile_insert_var(struct configfile_section *s,
                      const char *name, const char *value)
{
    struct pce_hashtable_entry *e;
    char *nvalue, *nname;
    size_t vlen, nlen;

    e = pce_hashtable_insert(&s->var, (char *) name);
    if (!e)
        return ENOMEM;
    if (!e->value) {
        nlen = strlen(name);
        nname = malloc(nlen + 1);
        if (!nname) {
            pce_hashtable_erase(&s->var, e);
            return ENOMEM;
        }
        memcpy(nname, name, nlen + 1);
        e->key = nname;
    } else {
        nname = NULL;
    }
    vlen = strlen(value);
    nvalue = malloc(vlen + 1);
    if (!nvalue) {
        if (nname) {
            free(nname);
            pce_hashtable_erase(&s->var, e);
        }
        return ENOMEM;
    }
    memcpy(nvalue, value, vlen + 1);
    if (e->value)
        free(e->value);
    e->value = nvalue;
    return 0;
}

void
configfile_erase_var(struct configfile_section *s,
                     struct pce_hashtable_entry *e)
{
    free(e->key);
    free(e->value);
    pce_hashtable_erase(&s->var, e);
}
