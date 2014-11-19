/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "private.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/type.h"

static const char SG_FONT_EXTENSIONS[] = "ttf:otf:woff";
#define SG_FONT_MAXSZ (16 * 1024 * 1024)

static FT_Library sg_font_library;

static void
sg_typeface_free(struct sg_typeface *fp)
{
    FT_Done_Face(fp->face);
    sg_filedata_decref(fp->data);
    free(fp);
}

void
sg_typeface_incref(struct sg_typeface *fp)
{
    fp->refcount++;
}

void
sg_typeface_decref(struct sg_typeface *fp)
{
    if (!--fp->refcount)
        sg_typeface_free(fp);
}

struct sg_typeface *
sg_typeface_file(const char *path, size_t pathlen, struct sg_error **err)
{
    FT_Error ferr;
    FT_Face face = NULL;
    FT_CharMap cmap;
    struct sg_filedata *data = NULL;
    int npathlen, i, n, r;
    char npath[SG_MAX_PATH], *pp;
    struct sg_typeface *tp;

    npathlen = sg_path_norm(npath, path, pathlen, err);
    if (npathlen < 0)
        return NULL;

    r = sg_file_load(&data, npath, npathlen, 0,
                     SG_FONT_EXTENSIONS, SG_FONT_MAXSZ, NULL, err);
    if (r)
        return NULL;

    if (!sg_font_library) {
        ferr = FT_Init_FreeType(&sg_font_library);
        if (ferr)
            goto freetype_error;
    }

    ferr = FT_New_Memory_Face(
        sg_font_library, data->data, (long) data->length, 0, &face);
    if (ferr)
        goto freetype_error;

    for (i = 0, n = face->num_charmaps; i < n; i++) {
        cmap = face->charmaps[i];
        if (cmap->encoding == FT_ENCODING_UNICODE) {
            ferr = FT_Set_Charmap(face, cmap);
            if (ferr)
                goto freetype_error;
            break;
        }
    }
    if (i == n) {
        sg_error_sets(err, &SG_ERROR_GENERIC, 0,
                      "no Unicode character map found");
        goto error;
    }

    tp = malloc(sizeof(*tp) + npathlen + 1);
    if (!tp) {
        sg_error_nomem(err);
        goto error;
    }

    pp = (char *) (tp + 1);
    tp->refcount = 1;
    tp->path = pp;
    tp->pathlen = npathlen;
    tp->data = data;
    tp->face = face;
    tp->font = NULL;
    memcpy(pp, npath, npathlen + 1);

    return tp;

freetype_error:
    sg_error_freetype(err, ferr);
    goto error;

error:
    if (face) FT_Done_Face(face);
    if (data) sg_filedata_decref(data);
    return NULL;
}
