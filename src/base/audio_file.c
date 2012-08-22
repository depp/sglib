#include "audio_file.h"
#include "audio_wav.h"
#include "dict.h"
#include "error.h"
#include "file.h"
#include "log.h"
#include "strbuf.h"
#include "thread.h"
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
    sg_lock_destroy(&fp->lock);
    switch (fp->type) {
    case SG_AUDIO_UNLOADED:
        break;

    case SG_AUDIO_PCM:
        sg_audio_pcm_destroy(&fp->data.pcm);
        break;
    }
    free(fp);
}

static void *
sg_audio_file_load(struct sg_resource *rs, struct sg_error **err)
{
    struct sg_audio_file *fp = (struct sg_audio_file *) rs;
    struct sg_buffer fbuf;
    int r;

    r = sg_file_get(fp->path, strlen(fp->path), 0, "wav", &fbuf,
                    AUDIO_MAXSIZE, err);
    if (r)
        return NULL;

    sg_lock_acquire(&fp->lock);

    if (sg_audio_file_checkwav(fbuf.data, fbuf.length)) {
        sg_audio_pcm_init(&fp->data.pcm);
        fp->type = SG_AUDIO_PCM;
        r = sg_audio_file_loadwav(
            &fp->data.pcm, fbuf.data, fbuf.length, err);
    } else {
        sg_logf(sg_audio_file_logger(), LOG_ERROR,
                "%s: unknown audio file format", fp->path);
        sg_error_data(err, "audio");
    }

    sg_lock_release(&fp->lock);

    return NULL;
}

static void
sg_audio_file_load_finished(struct sg_resource *rs, void *result)
{
    /* We don't do anything here, since we use a mutex to share
       results with other threads.  */
    (void) rs;
    (void) result;
}

static void
sg_audio_file_get_name(struct sg_resource *rs, struct sg_strbuf *buf)
{
    struct sg_audio_file *fp = (struct sg_audio_file *) rs;
    sg_strbuf_printf(buf, "audio:%s", fp->path);
}

static const struct sg_resource_type sg_audio_file_type = {
    sg_audio_file_free,
    sg_audio_file_load,
    sg_audio_file_load_finished,
    sg_audio_file_get_name,
    SG_DISPATCH_IO
};

static struct dict sg_audio_files;

struct sg_audio_file *
sg_audio_file_new(const char *path, struct sg_error **err)
{
    struct dict_entry *e;
    struct sg_audio_file *fp;
    size_t len;
    char *pp;

    e = dict_insert(&sg_audio_files, (char *) path);
    fp = e->value;
    if (!fp) {
        len = strlen(path);
        fp = malloc(sizeof(*fp) + len + 1);
        if (!fp) {
            dict_erase(&sg_audio_files, e);
            sg_error_nomem(err);
            return NULL;
        }
        pp = (char *) (fp + 1);
        fp->r.type = &sg_audio_file_type;
        fp->r.refcount = 0;
        fp->r.action = 0;
        fp->r.state = SG_RSRC_INITIAL;
        fp->path = pp;
        sg_lock_init(&fp->lock);
        fp->type = SG_AUDIO_UNLOADED;
        memcpy(pp, path, len + 1);
        e->key = pp;
        e->value = fp;
    }
    sg_resource_incref(&fp->r);

    return fp;
}
