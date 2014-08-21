/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/audio_buffer.h"
#include "sg/defs.h"
#include "sg/error.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>

static void
sg_audio_buffer_resample_1(short *SG_RESTRICT dest, int dlen, int drate,
                           const short *SG_RESTRICT src, int slen, int srate)
{
    double rr;
    int i, pos, x;

    rr = (double) srate / drate;
    for (i = 0; i < dlen; ++i) {
        pos = (int) (rr * i);
        if (pos < 0 || pos >= slen)
            x = 0;
        else
            x = src[pos];
        dest[i] = x;
    }
}

static void
sg_audio_buffer_resample_2(short *SG_RESTRICT dest, int dlen, int drate,
                           const short *SG_RESTRICT src, int slen, int srate)
{
    double rr;
    int i, pos, x0, x1;

    rr = (double) srate / drate;
    for (i = 0; i < dlen; ++i) {
        pos = (int) (rr * i);
        if (pos < 0 || pos >= slen) {
            x0 = 0;
            x1 = 0;
        } else {
            x0 = src[pos*2+0];
            x1 = src[pos*2+1];
        }
        dest[i*2+0] = x0;
        dest[i*2+1] = x1;
    }
}

int
sg_audio_buffer_resample(struct sg_audio_buffer *buf,
                         int rate,
                         struct sg_error **err)
{
    void (*func)(short *SG_RESTRICT, int, int,
                 const short *SG_RESTRICT, int, int);
    size_t nlensz;
    int nlen, nchan;
    double fnlen;
    short *dest;

    if (buf->nframe <= 0) {
        buf->rate = rate;
        return 0;
    }

    nchan = buf->nchan;
    if (buf->format != SG_AUDIO_S16NE || buf->rate <= 0)
        goto invalid;

    switch (nchan) {
    case 1:
        func = sg_audio_buffer_resample_1;
        break;

    case 2:
        func = sg_audio_buffer_resample_2;
        break;

    default:
        goto invalid;
    }

    fnlen = floor(buf->nframe * (double) rate / buf->rate + 0.5);
    if (fnlen > INT_MAX)
        goto nomem;
    nlen = (int) fnlen;
    if (nlen <= 0) {
        free(buf->alloc);
        buf->alloc = NULL;
        buf->data = NULL;
        buf->rate = rate;
        buf->nframe = 0;
        return 0;
    }
    nlensz = nlen;
    if (nlensz > (size_t) -1 / nchan)
        goto nomem;

    dest = malloc(sizeof(short) * nchan * nlen);
    if (!dest)
        goto nomem;

    func(dest, nlen, rate, buf->data, buf->nframe, buf->rate);

    free(buf->alloc);
    buf->alloc = dest;
    buf->data = dest;
    buf->rate = rate;
    buf->nframe = nlen;
    return 0;

nomem:
    sg_error_nomem(err);
    return -1;

invalid:
    sg_error_invalid(err, __FUNCTION__, "buf");
    return -1;
}
