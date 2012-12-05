/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "fileprivate.h"
#include "libpce/byteorder.h"
#include "sg/audio_file.h"
#include "sg/error.h"
#include "sg/log.h"
#include "sysprivate.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void
sg_audio_copy_u8(short *SG_RESTRICT dest, const void *SG_RESTRICT src,
                 size_t count)
{
    const unsigned char *SG_RESTRICT sp;
    size_t i;

    sp = src;
    for (i = 0; i < count; ++i)
        dest[i] = (sp[i] ^ 0x80) << 8;
}

static void
sg_audio_copy_s16(short *SG_RESTRICT dest, const void *SG_RESTRICT src,
                  size_t count, int littleendian)
{
    const unsigned char *SG_RESTRICT sp;
    size_t i;

    if (littleendian == (PCE_BYTE_ORDER == PCE_LITTLE_ENDIAN)) {
        memcpy(dest, src, count * sizeof(short));
        return;
    }

    sp = src;
    for (i = 0; i < count; ++i) {
        if (BYTE_ORDER == LITTLE_ENDIAN)
            dest[i] = (sp[i*2] << 8) | sp[i*2+1];
        else
            dest[i] = sp[i*2] | (sp[i*2+1] << 8);
    }
}

static void
sg_audio_copy_s24(short *SG_RESTRICT dest, const void *SG_RESTRICT src,
                  size_t count, int littleendian)
{
    const unsigned char *SG_RESTRICT sp;
    size_t i;

    sp = src;
    if (littleendian) {
        for (i = 0; i < count; ++i)
            dest[i] = sp[i*3] | (sp[i*3+1] << 8);
    } else {
        for (i = 0; i < count; ++i)
            dest[i] = sp[i*3+2] | (sp[i*3+1] << 8);
    }
}

static void
sg_audio_copy_f32(short *SG_RESTRICT dest, const void *SG_RESTRICT src,
                  size_t count, int littleendian)
{
    const unsigned char *SG_RESTRICT sp;
    size_t i;
    union {
        float f;
        unsigned char c[4];
    } x;

    sp = src;
    if (littleendian == (BYTE_ORDER == LITTLE_ENDIAN)) {
        for (i = 0; i < count; ++i) {
            x.c[0] = sp[i*4+0];
            x.c[1] = sp[i*4+1];
            x.c[2] = sp[i*4+2];
            x.c[3] = sp[i*4+3];
            dest[i] = x.f * 32767.0f;
        }
    } else {
        for (i = 0; i < count; ++i) {
            x.c[3] = sp[i*4+0];
            x.c[2] = sp[i*4+1];
            x.c[1] = sp[i*4+2];
            x.c[0] = sp[i*4+3];
            dest[i] = x.f * 32767.0f;
        }
    }
}

int
sg_audio_file_loadraw(struct sg_audio_file *fp,
                      const void *data, size_t count,
                      sg_audio_format_t format, int nchan, int rate,
                      struct sg_error **err)
{
    size_t numsamp;
    int i;
    short *ndata;

    sg_audio_file_clear(fp);

    if (nchan < 1 || nchan > 2) {
        sg_logf(sg_audio_system_global.log, LOG_ERROR,
                "audio file has unsupported number of channels (%d)",
                nchan);
        sg_error_data(err, "audio");
        return -1;
    }

    if (count < 1) {
        ndata = malloc(sizeof(short) * nchan);
        if (!ndata) {
            sg_error_nomem(err);
            return -1;
        }
        for (i = 0; i < nchan; ++i)
            ndata[i] = 0;
        fp->flags = nchan == 2 ? SG_AUDIOFILE_STEREO : 0;
        fp->nframe = count;
        fp->playtime = 0;
        fp->rate = rate;
        fp->data = ndata;
        return 0;
    }

    numsamp = count * nchan;
    ndata = malloc(sizeof(*ndata) * numsamp);
    if (!ndata) {
        sg_error_nomem(err);
        return -1;
    }

    if (fp->data) {
        free(fp->data);
        fp->data = NULL;
        fp->flags = 0;
    }

    switch (format) {
    case SG_AUDIO_U8:
        sg_audio_copy_u8(ndata, data, numsamp);
        break;

    case SG_AUDIO_S16BE:
    case SG_AUDIO_S16LE:
        sg_audio_copy_s16(ndata, data, numsamp, format == SG_AUDIO_S16LE);
        break;

    case SG_AUDIO_S24BE:
    case SG_AUDIO_S24LE:
        sg_audio_copy_s24(ndata, data, numsamp, format == SG_AUDIO_S24LE);
        break;

    case SG_AUDIO_F32BE:
    case SG_AUDIO_F32LE:
        sg_audio_copy_f32(ndata, data, numsamp, format == SG_AUDIO_F32LE);
        break;

    default:
        assert(0);
    }

    /* FIXME: FIXMEATOMIC: */
    fp->flags = nchan == 2 ? SG_AUDIOFILE_STEREO : 0;
    fp->nframe = count;
    fp->playtime = (int) ceil(count / (double) (rate * 0.001));
    fp->rate = rate;
    fp->data = ndata;

    return 0;
}
