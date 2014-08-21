/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "libpce/util.h"
#include "ogg.h"
#include "sg/audio_buffer.h"
#include "sg/error.h"
#include "sg/log.h"
#include <math.h>
#include <opus.h>
#include <stdlib.h>
#include <string.h>

/* Opus really runs at this sampling rate, or at simple decimations
   thereof (24 kHz, 16 kHz, 12 kHz, 8 kHz).  */
#define SG_OPUS_RATE 48000

/* Maximum Opus frame is 120ms, frequency is 48000 Hz.  */
#define SG_OPUS_MAXFRAME (48 * 120)

static void
sg_opus_log(const char *msg)
{
    sg_logf(sg_logger_get("audio"), SG_LOG_ERROR,
            "Opus error: %s", msg);
}

static void
sg_opus_logerr(int code)
{
    const char *ename;
    switch (code) {
    case OPUS_BAD_ARG: ename = "BAD_ARG"; break;
    case OPUS_BUFFER_TOO_SMALL: ename = "BUFFER_TOO_SMALL"; break;
    case OPUS_INTERNAL_ERROR: ename = "INTERNAL_ERROR"; break;
    case OPUS_INVALID_PACKET: ename = "INVALID_PACKET"; break;
    case OPUS_UNIMPLEMENTED: ename = "UNIMPLEMENTED"; break;
    case OPUS_INVALID_STATE: ename = "INVALID_STATE"; break;
    case OPUS_ALLOC_FAIL: ename = "ALLOC_FAIL"; break;
    default: ename = "<unknown error>"; break;
    }
    sg_opus_log(ename);
}

struct sg_opus_decoder {
    int state; /* = number of header packets decoded, 0..2 */

    OpusDecoder *decoder;
    int preskip;
    int channels;
    float gain;

    float *buf;
    size_t buflen;
    size_t bufalloc;
};

void *
sg_opus_decoder_new(struct sg_error **err)
{
    struct sg_opus_decoder *st;
    st = malloc(sizeof(*st));
    if (!st) {
        sg_error_nomem(err);
        return NULL;
    }

    st->state = 0;
    st->decoder = NULL;
    st->buf = NULL;
    st->buflen = 0;
    st->bufalloc = 0;

    return st;
}

void
sg_opus_decoder_free(void *obj)
{
    struct sg_opus_decoder *st = obj;
    if (st->decoder)
        opus_decoder_destroy(st->decoder);
    free(st->buf);
    free(st);
}

int
sg_opus_decoder_packet(void *obj, ogg_packet *op,
                       struct sg_error **err)
{
    struct sg_opus_decoder *st = obj;
    unsigned char *data = op->packet;
    size_t len = op->bytes;
    const char *msg;
    int r;

    if (st->state < 2) {
        if (st->state == 0) {
            int version, channels, preskip, chanmap, gain;
            version = data[9];
            if ((version >> 4) != 0) {
                msg = "Unknown version";
                goto custom_error;
            }
            channels = data[9];
            if (channels < 1 || channels > 2) {
                msg = "Unsupported number of channels";
                goto custom_error;
            }
            preskip = data[10] | (data[11] << 8);
            chanmap = data[18];
            if (chanmap != 0) {
                msg = "Unsupported channel map";
                goto custom_error;
            }
            gain = (short) (data[32] | (data[33] << 8));
            st->gain = expf((float) (log(10.0) / (20 * 256)) *
                            (float) gain);
            st->state = 1;
            st->preskip = preskip;
            st->channels = channels;

            st->decoder = opus_decoder_create(SG_OPUS_RATE, channels, &r);
            if (r)
                goto opus_error;
        } else {
            /* Comments are ignored */
            st->state = 2;
        }
    } else {
        size_t nalloc;
        int channels;
        float *nbuf;
        channels = st->channels;
        if (SG_OPUS_MAXFRAME > st->bufalloc - st->buflen) {
            nalloc = pce_round_up_pow2(st->buflen + SG_OPUS_MAXFRAME);
            nbuf = realloc(st->buf, sizeof(float) * channels * nalloc);
            if (!nbuf) {
                sg_error_nomem(err);
                return -1;
            }
            st->buf = nbuf;
            st->bufalloc = nalloc;
        }
        r = opus_decode_float(st->decoder, data, len, st->buf + st->buflen,
                              SG_OPUS_MAXFRAME, 0);
        if (r < 0)
            goto opus_error;
        if (st->preskip > 0) {
            if (st->preskip < r) {
                memmove(st->buf, st->buf + st->preskip * channels,
                        sizeof(float) * channels * (r - st->preskip));
                st->buflen = r - st->preskip;
                st->preskip = 0;
            } else {
                st->preskip -= r;
            }
        } else {
            st->buflen += r;
        }
    }
    return 0;

custom_error:
    sg_opus_log(msg);
    sg_error_data(err, "opus");
    return -1;

opus_error:
    sg_opus_logerr(r);
    sg_error_data(err, "opus");
    return -1;
}

int
sg_opus_decoder_read(void *obj, struct sg_audio_buffer *pcm,
                       struct sg_error **err)
{
    struct sg_opus_decoder *st = obj;
    float *buf, gain;
    size_t i, len;

    if (st->state < 2) {
        sg_opus_log("Incomplete header");
        sg_error_data(err, "opus");
        return -1;
    }

    if (st->buflen < st->bufalloc) {
        buf = realloc(st->buf, sizeof(float) * st->channels * st->buflen);
        if (!buf)
            abort();
    } else {
        buf = st->buf;
    }

    if (st->gain != 1.0f) {
        gain = st->gain;
        len = st->buflen * st->channels;
        for (i = 0; i < len; i++)
            buf[i] *= gain;
    }

    pcm->alloc = buf;
    pcm->data = buf;
    pcm->format = SG_AUDIO_F32NE;
    pcm->rate = SG_OPUS_RATE;
    pcm->nchan = st->channels;
    pcm->nframe = (int) st->buflen;

    st->buf = NULL;
    st->buflen = 0;
    st->bufalloc = 0;
    return 0;
}
