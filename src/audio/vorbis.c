/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/util.h"
#include "ogg.h"
#include "sg/audio_buffer.h"
#include "sg/error.h"
#include "sg/log.h"
#include <vorbis/codec.h>
#include <stdlib.h>
#include <string.h>

static void
sg_vorbis_log(const char *msg)
{
    sg_logf(sg_logger_get("audio"), SG_LOG_ERROR,
            "Vorbis error: %s", msg);
}

static void
sg_vorbis_logerr(int code)
{
    const char *ename;
    switch (code) {
    case OV_EREAD: ename = "EREAD"; break;
    case OV_EFAULT: ename = "EFAULT"; break;
    case OV_EIMPL: ename = "EIMPL"; break;
    case OV_EINVAL: ename = "EINVAL"; break;
    case OV_ENOTVORBIS: ename = "ENOTVORBIS"; break;
    case OV_EBADHEADER: ename = "EBADHEADER"; break;
    case OV_EVERSION: ename = "EVERSION"; break;
    case OV_ENOTAUDIO: ename = "ENOTAUDIO"; break;
    case OV_EBADPACKET: ename = "EBADPACKET"; break;
    case OV_EBADLINK: ename = "EBADLINK"; break;
    case OV_ENOSEEK: ename = "ENOSEEK"; break;
    default: ename = "<unknown error>"; break;
    }
    sg_vorbis_log(ename);
}

struct sg_vorbis_decoder {
    int state; /* = number of header packets decoded, 0..3 */

    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state v;
    vorbis_block vb;
    int channels;

    float *buf;
    size_t buflen;
    size_t bufalloc;
};

void *
sg_vorbis_decoder_new(struct sg_error **err)
{
    struct sg_vorbis_decoder *st;
    st = malloc(sizeof(*st));
    if (!st) {
        sg_error_nomem(err);
        return NULL;
    }

    st->state = 0;
    vorbis_info_init(&st->vi);
    vorbis_comment_init(&st->vc);
    st->buf = NULL;
    st->buflen = 0;
    st->bufalloc = 0;

    return st;
}

void
sg_vorbis_decoder_free(void *obj)
{
    struct sg_vorbis_decoder *st = obj;
    if (st->state == 3) {
        vorbis_block_clear(&st->vb);
        vorbis_dsp_clear(&st->v);
    }
    vorbis_comment_clear(&st->vc);
    vorbis_info_clear(&st->vi);
    free(st->buf);
    free(st);
}

int
sg_vorbis_decoder_packet(void *obj, ogg_packet *op,
                         struct sg_error **err)
{
    const char *msg;
    struct sg_vorbis_decoder *st = obj;
    int r, nsamp, i;
    size_t nalloc;
    float **pcm, *nbuf;

    if (st->state < 3) {
        r = vorbis_synthesis_headerin(&st->vi, &st->vc, op);
        if (r)
            goto vorbis_error;

        if (st->state == 2) {
            if (st->vi.channels < 1 || st->vi.channels > 2) {
                msg = "Unsupported number of channels";;
                goto custom_error;
            }
            st->channels = st->vi.channels;

            r = vorbis_synthesis_init(&st->v, &st->vi);
            if (r)
                goto vorbis_error;
            r = vorbis_block_init(&st->v, &st->vb);
            if (r) {
                vorbis_dsp_clear(&st->v);
                goto vorbis_error;
            }
        }

        st->state++;
    } else {
        r = vorbis_synthesis(&st->vb, op);
        if (r)
            goto vorbis_error;
        r = vorbis_synthesis_blockin(&st->v, &st->vb);
        if (r)
            goto vorbis_error;
        nsamp = vorbis_synthesis_pcmout(&st->v, &pcm);

        if ((size_t) nsamp > st->bufalloc - st->buflen) {
            nalloc = sg_round_up_pow2(st->buflen + nsamp);
            nbuf = realloc(st->buf, sizeof(float) * st->channels * nalloc);
            if (!nbuf) {
                sg_error_nomem(err);
                return -1;
            }
            st->buf = nbuf;
            st->bufalloc = nalloc;
        }

        switch (st->channels) {
        case 1:
            memcpy(st->buf + st->buflen, pcm[0], sizeof(float) * nsamp);
            break;

        case 2:
            for (i = 0; i < nsamp; i++) {
                st->buf[(st->buflen+i)*2+0] = pcm[0][i];
                st->buf[(st->buflen+i)*2+1] = pcm[1][i];
            }
            break;
        }

        st->buflen += nsamp;
        r = vorbis_synthesis_read(&st->v, nsamp);
        if (r)
            goto vorbis_error;
    }
    return 0;

custom_error:
    sg_vorbis_log(msg);
    sg_error_data(err, "vorbis");
    return -1;

vorbis_error:
    sg_vorbis_logerr(r);
    sg_error_data(err, "vorbis");
    return -1;
}

int
sg_vorbis_decoder_read(void *obj, struct sg_audio_buffer *pcm,
                       struct sg_error **err)
{
    struct sg_vorbis_decoder *st = obj;
    void *buf;

    if (st->state < 3) {
        sg_vorbis_log("Incomplete header");
        sg_error_data(err, "vorbis");
        return -1;
    }

    if (st->buflen < st->bufalloc) {
        buf = realloc(st->buf, sizeof(float) * st->channels * st->buflen);
        if (!buf)
            abort();
    } else {
        buf = st->buf;
    }

    pcm->alloc = buf;
    pcm->data = buf;
    pcm->format = SG_AUDIO_F32NE;
    pcm->rate = st->vi.rate;
    pcm->nchan = st->channels;
    pcm->nframe = (int) st->buflen;

    st->buf = NULL;
    st->buflen = 0;
    st->bufalloc = 0;
    return 0;
}
