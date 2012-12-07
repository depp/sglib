/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "fileprivate.h"
#include "libpce/hashtable.h"
#include "libpce/strbuf.h"
#include "libpce/thread.h"
#include "sg/audio_file.h"
#include "sg/error.h"
#include "sg/file.h"
#include "sg/log.h"
#include "sysprivate.h"
#include <stdlib.h>
#include <string.h>

/* Maximum size of an audio file in bytes.  */
#define AUDIO_MAXSIZE (16 * 1024 * 1024)

static struct sg_logger *
sg_audio_file_logger(void)
{
    return sg_logger_get("audio");
}

static void
sg_audio_file_free(struct sg_resource *rs)
{
    struct sg_audio_file *fp = (struct sg_audio_file *) rs;
    free(fp->data);
    free(fp);
}

static void *
sg_audio_file_load(struct sg_resource *rs, struct sg_error **err)
{
    struct sg_audio_file *fp = (struct sg_audio_file *) rs;
    struct sg_buffer *fbuf;
    int r, rate;

    /* FIXME: FIXMEATOMIC: */

    assert((fp->flags & SG_AUDIOFILE_LOADED) == 0);

    if ((fp->flags & SG_AUDIOFILE_RESAMPLED) != 0)
        sg_audio_file_clear(fp);

    if (!fp->data) {
        fbuf = sg_file_get(fp->path, strlen(fp->path), 0, "wav",
                           AUDIO_MAXSIZE, err);
        if (!fbuf)
            return NULL;

        if (sg_audio_file_checkwav(fbuf->data, fbuf->length)) {
            r = sg_audio_file_loadwav(fp, fbuf->data, fbuf->length, err);
            sg_buffer_decref(fbuf);
        } else {
            sg_buffer_decref(fbuf);
            sg_logf(sg_audio_file_logger(), LOG_ERROR,
                    "%s: unknown audio file format", fp->path);
            sg_error_data(err, "audio");
            return NULL;
        }
    }

    rate = sg_audio_system_global.samplerate;
    if (rate && fp->rate != rate) {
        r = sg_audio_file_resample(fp, rate, err);
        if (r)
            return NULL;
    }

    return NULL;
}

static void
sg_audio_file_load_finished(struct sg_resource *rs, void *result)
{
    struct sg_audio_file *fp = (struct sg_audio_file *) rs;
    int rate;

    if (!fp->data)
        return;

    (void) result;

    rate = sg_audio_system_global.samplerate;
    if (fp->rate == rate) {
        /* FIXME: FIXMEATOMIC: */
        fp->flags |= SG_AUDIOFILE_LOADED;
    } else if (rate) {
        fp->r.state = SG_RSRC_UNLOADED;
    }
    /* If rate == 0, we just leave the file at its existing sample
       rate, but we don't mark the file as loaded */
}

static void
sg_audio_file_get_name(struct sg_resource *rs, struct pce_strbuf *buf)
{
    struct sg_audio_file *fp = (struct sg_audio_file *) rs;
    pce_strbuf_printf(buf, "audio:%s", fp->path);
}

static const struct sg_resource_type sg_audio_file_type = {
    sg_audio_file_free,
    sg_audio_file_load,
    sg_audio_file_load_finished,
    sg_audio_file_get_name
};

static struct pce_hashtable sg_audio_files;

struct sg_audio_file *
sg_audio_file_new(const char *path, struct sg_error **err)
{
    struct pce_hashtable_entry *e;
    struct sg_audio_file *fp;
    size_t len;
    char *pp;

    e = pce_hashtable_insert(&sg_audio_files, (char *) path);
    fp = e->value;
    if (!fp) {
        len = strlen(path);
        fp = malloc(sizeof(*fp) + len + 1);
        if (!fp) {
            pce_hashtable_erase(&sg_audio_files, e);
            sg_error_nomem(err);
            return NULL;
        }
        pp = (char *) (fp + 1);
        fp->r.type = &sg_audio_file_type;
        fp->r.refcount = 0;
        fp->r.action = 0;
        fp->r.state = SG_RSRC_INITIAL;
        fp->path = pp;
        fp->flags = 0;
        fp->nframe = 0;
        fp->rate = 0;
        fp->data = NULL;
        memcpy(pp, path, len + 1);
        e->key = pp;
        e->value = fp;
    }
    sg_resource_incref(&fp->r);

    return fp;
}

void
sg_audio_file_clear(struct sg_audio_file *fp)
{
    free(fp->data);
    fp->flags = 0;
    fp->nframe = 0;
    fp->playtime = 0;
    fp->rate = 0;
    fp->data = NULL;
}
