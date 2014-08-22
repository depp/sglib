/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/audio_buffer.h"
#include "sg/defs.h"
#include "sg/error.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void
sg_audio_pcm_copy_u8(short *SG_RESTRICT dest, const void *SG_RESTRICT src,
                     size_t count)
{
    const unsigned char *SG_RESTRICT sp;
    unsigned x;
    size_t i;

    sp = src;
    for (i = 0; i < count; ++i) {
        x = sp[i];
        x |= x << 8;
        dest[i] = x ^ 0x8000;
    }
}

static void
sg_audio_pcm_copy_s16(short *dest, const void *src,
                      size_t count, int littleendian)
{
    const unsigned char *sp;
    size_t i;

    if (littleendian == (SG_BYTE_ORDER == SG_LITTLE_ENDIAN)) {
        if (dest != src)
            memcpy(dest, src, count * sizeof(short));
        return;
    }

    sp = src;
    for (i = 0; i < count; ++i) {
        if (SG_BYTE_ORDER == SG_LITTLE_ENDIAN)
            dest[i] = (sp[i*2] << 8) | sp[i*2+1];
        else
            dest[i] = sp[i*2] | (sp[i*2+1] << 8);
    }
}

static void
sg_audio_pcm_copy_s24(short *SG_RESTRICT dest, const void *SG_RESTRICT src,
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
sg_audio_pcm_copy_f32(short *SG_RESTRICT dest, const void *SG_RESTRICT src,
                      size_t count, int littleendian)
{
    const unsigned char *SG_RESTRICT sp;
    size_t i;
    union {
        float f;
        unsigned char c[4];
    } x;

    sp = src;
    if (littleendian == (SG_BYTE_ORDER == SG_LITTLE_ENDIAN)) {
        for (i = 0; i < count; ++i) {
            x.c[0] = sp[i*4+0];
            x.c[1] = sp[i*4+1];
            x.c[2] = sp[i*4+2];
            x.c[3] = sp[i*4+3];
            dest[i] = (short) (x.f * 32767.0f);
        }
    } else {
        for (i = 0; i < count; ++i) {
            x.c[3] = sp[i*4+0];
            x.c[2] = sp[i*4+1];
            x.c[1] = sp[i*4+2];
            x.c[0] = sp[i*4+3];
            dest[i] = (short) (x.f * 32767.0f);
        }
    }
}

int
sg_audio_buffer_convert(struct sg_audio_buffer *buf,
                        sg_audio_format_t format,
                        struct sg_error **err)
{
    size_t nsamp, osz, nsz;
    void *dest;
    const void *src;

    assert(format == SG_AUDIO_S16NE);

    nsamp = (size_t) buf->nframe * buf->nchan;
    osz = sg_audio_format_size(buf->format);
    nsz = sg_audio_format_size(format);
    if (buf->alloc && nsz == osz) {
        dest = buf->alloc;
        src = dest;
    } else {
        dest = malloc(nsz * nsamp);
        if (!dest) {
            sg_error_nomem(err);
            return -1;
        }
        src = buf->data;
    }

    switch (buf->format) {
    case SG_AUDIO_U8:
        sg_audio_pcm_copy_u8(dest, src, nsamp);
        break;

    case SG_AUDIO_S16BE:
    case SG_AUDIO_S16LE:
        sg_audio_pcm_copy_s16(dest, src, nsamp,
                              buf->format == SG_AUDIO_S16LE);
        break;

    case SG_AUDIO_S24BE:
    case SG_AUDIO_S24LE:
        sg_audio_pcm_copy_s24(dest, src, nsamp,
                              buf->format == SG_AUDIO_S24LE);
        break;

    case SG_AUDIO_F32BE:
    case SG_AUDIO_F32LE:
        sg_audio_pcm_copy_f32(dest, src, nsamp,
                              buf->format == SG_AUDIO_F32LE);
        break;
    }

    if (dest != src) {
        free(buf->alloc);
        buf->alloc = dest;
        buf->data = dest;
    }
    buf->format = format;

    return 0;
}
