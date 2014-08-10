/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/configfile.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

void
sg_configfile_init(struct sg_configfile *file)
{
    pce_hashtable_init(&file->sect);
}

static void
sg_configfile_section_free(struct sg_configfile_section *section)
{
    struct pce_hashtable_entry
        *p = section->var.contents, *e = p + section->var.capacity;
    for (; p != e; ++p) {
        if (p->key) {
            free(p->key);
            free(p->value);
        }
    }
    free(section->var.contents);
    free(section->name);
    free(section);
}

void
sg_configfile_destroy(struct sg_configfile *file)
{
    struct pce_hashtable_entry
        *p = file->sect.contents, *e = p + file->sect.capacity;
    for (; p != e; ++p) {
        if (p->key)
            sg_configfile_section_free(p->value);
    }
    free(file->sect.contents);
}

const char *
sg_configfile_get(struct sg_configfile *file, const char *section,
                  const char *name)
{
    struct pce_hashtable_entry *e;
    struct sg_configfile_section *s;
    e = pce_hashtable_get(&file->sect, section);
    if (!e)
        return NULL;
    s = e->value;
    e = pce_hashtable_get(&s->var, name);
    if (!e)
        return NULL;
    return e->value;
}

int
sg_configfile_set(struct sg_configfile *file, const char *section,
                  const char *name, const char *value)
{
    struct sg_configfile_section *s;
    s = sg_configfile_insert_section(file, section);
    if (!s)
        return -1;
    return sg_configfile_insert_var(s, name, value);
}

void
sg_configfile_remove(struct sg_configfile *file,
                     const char *section, const char *name)
{
    struct pce_hashtable_entry *se, *ve;
    struct sg_configfile_section *s;
    se = pce_hashtable_get(&file->sect, section);
    if (!se)
        return;
    s = se->value;
    ve = pce_hashtable_get(&s->var, name);
    if (!ve)
        return;
    sg_configfile_erase_var(s, ve);
    if (!s->var.size) {
        sg_configfile_section_free(s);
        pce_hashtable_erase(&file->sect, se);
    }
}

struct sg_configfile_section *
sg_configfile_insert_section(struct sg_configfile *file, const char *name)
{
    struct pce_hashtable_entry *e;
    struct sg_configfile_section *s;
    char *nname;
    size_t len;
    e = pce_hashtable_insert(&file->sect, (char *) name);
    if (!e)
        return NULL;
    if (e->value)
        return e->value;
    len = strlen(name);
    nname = malloc(len + 1);
    if (!nname) {
        pce_hashtable_erase(&file->sect, e);
        return NULL;
    }
    memcpy(nname, name, len + 1);
    s = malloc(sizeof(*s));
    if (!s) {
        pce_hashtable_erase(&file->sect, e);
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
sg_configfile_erase_section(struct sg_configfile *file,
                            struct sg_configfile_section *section)
{
    struct pce_hashtable_entry *e;
    e = pce_hashtable_get(&file->sect, section->name);
    if (!e)
        return;
    assert(e->value == section);
    sg_configfile_section_free(section);
    pce_hashtable_erase(&file->sect, e);
}

int
sg_configfile_insert_var(struct sg_configfile_section *section,
                         const char *name, const char *value)
{
    struct pce_hashtable_entry *e;
    char *nvalue, *nname;
    size_t vlen, nlen;

    e = pce_hashtable_insert(&section->var, (char *) name);
    if (!e)
        return -1;
    if (!e->value) {
        nlen = strlen(name);
        nname = malloc(nlen + 1);
        if (!nname) {
            pce_hashtable_erase(&section->var, e);
            return -1;
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
            pce_hashtable_erase(&section->var, e);
        }
        return -1;
    }
    memcpy(nvalue, value, vlen + 1);
    if (e->value)
        free(e->value);
    e->value = nvalue;
    return 0;
}

void
sg_configfile_erase_var(struct sg_configfile_section *section,
                        struct pce_hashtable_entry *entry)
{
    free(entry->key);
    free(entry->value);
    pce_hashtable_erase(&section->var, entry);
}
