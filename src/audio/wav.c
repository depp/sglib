/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/binary.h"
#include "sg/audio_buffer.h"
#include "sg/audio_file.h"
#include "sg/error.h"
#include "sg/log.h"
#include <stdlib.h>
#include <string.h>

/**********************************************************************/
/* RIFF parsing */

struct sg_riff_tag {
    char tag[4];
    uint32_t length;
    const void *data;
};

struct sg_riff {
    char tag[4];
    unsigned atags, ntags;
    struct sg_riff_tag *tags;
};

/* Parse a RIFF file into an uninitialized structure.  Return 0 if
   successful, nonzero on failure.  */
static int
sg_riff_parse(struct sg_riff *riff, const void *data, size_t length,
              struct sg_error **err)
{
    const unsigned char *p = data;
    uint32_t rlength, pos;
    struct sg_riff_tag t, *tp;
    unsigned nalloc;

    riff->atags = 0;
    riff->ntags = 0;
    riff->tags = NULL;

    if (length < 8)
        goto fmterr;
    if (memcmp(p, "RIFF", 4))
        goto fmterr;
    rlength = sg_read_lu32(p + 4);
    if (rlength > length - 8 || rlength < 4)
        goto fmterr;
    memcpy(riff->tag, p + 8, 4);

    p += 12;
    rlength -= 4;
    if (!rlength)
        return 0;
    if (rlength < 8)
        goto fmterr;
    pos = 0;
    while (pos < rlength) {
        if (pos > rlength - 8)
            goto fmterr;
        memcpy(&t.tag, p + pos, 4);
        t.length = sg_read_lu32(p + pos + 4);
        pos += 8;
        if (t.length > rlength - pos)
            goto fmterr;
        t.data = p + pos;
        pos += t.length;

        if (riff->ntags >= riff->atags) {
            nalloc = riff->atags ? riff->atags * 2 : 2;
            tp = realloc(riff->tags, sizeof(*tp) * nalloc);
            if (!tp)
                goto memerr;
            riff->tags = tp;
            riff->atags = nalloc;
        }
        memcpy(&riff->tags[riff->ntags++], &t, sizeof(t));

        if (pos & 1) {
            /* Guaranteed not to overflow,
               since we subtracted 4 from rlength */
            pos += 1;
        }
    }
    return 0;

fmterr:
    sg_error_data(err, "RIFF");
    free(riff->tags);
    return -1;

memerr:
    sg_error_nomem(err);
    free(riff->tags);
    return -1;
}

static void
sg_riff_destroy(struct sg_riff *riff)
{
    free(riff->tags);
}

static struct sg_riff_tag *
sg_riff_get(struct sg_riff *riff, const char tag[4])
{
    struct sg_riff_tag *p = riff->tags, *e = p + riff->ntags;
    for (; p != e; ++p)
        if (!memcmp(p->tag, tag, 4))
            return p;
    return NULL;
}

/**********************************************************************/
/* WAVE parsing */

/* Wave audio fomats we understand */
enum {
    SG_WAVE_PCM = 1,
    SG_WAVE_FLOAT = 3
};

int
sg_audio_file_loadwav(struct sg_audio_buffer *buf,
                      const void *data, size_t len,
                      struct sg_error **err)
{
    struct sg_riff riff;
    struct sg_riff_tag *tag;
    int r;
    uint16_t afmt, nchan, sampbits;
    uint32_t rate, nframe;
    const unsigned char *p;
    sg_audio_format_t format;

    r = sg_riff_parse(&riff, data, len, err);
    if (r)
        return -1;

    if (memcmp(riff.tag, "WAVE", 4))
        goto fmterr;

    /* Read WAVE format information */
    tag = sg_riff_get(&riff, "fmt ");
    if (!tag)
        goto fmterr;
    if (tag->length < 16)
        goto fmterr;
    p = tag->data;
    afmt = sg_read_lu16(p + 0);
    nchan = sg_read_lu16(p + 2);
    rate = sg_read_lu32(p + 4);
    /* blkalign = sg_read_lu16(p + 12); */
    sampbits = sg_read_lu16(p + 14);
    if (rate < SG_AUDIO_BUFFER_MINRATE || rate > SG_AUDIO_BUFFER_MAXRATE) {
        sg_logf(SG_LOG_ERROR, "WAVE sample rate too extreme (%u Hz)", rate);
        goto fmterr;
    }
    if (nchan != 1 && nchan != 2) {
        goto fmterr;
    }

    /* Read WAVE data */
    tag = sg_riff_get(&riff, "data");
    if (!tag)
        goto fmterr;
    p = tag->data;

    switch (afmt) {
    case SG_WAVE_PCM:
        switch (sampbits) {
        case 8:
            nframe = tag->length / nchan;
            format = SG_AUDIO_U8;
            break;

        case 16:
            nframe = tag->length / (2 * nchan);
            format = SG_AUDIO_S16LE;
            break;

        case 24:
            nframe = tag->length / (3 * nchan);
            format = SG_AUDIO_S24LE;
            break;

        default:
            sg_logf(SG_LOG_ERROR, "invalid WAVE bit depth: %d", sampbits);
            goto fmterr;
        }
        break;

    case SG_WAVE_FLOAT:
        if (sampbits != 32) {
            sg_logs(SG_LOG_ERROR, "WAVE float bits != 32");
            goto fmterr;
        }
        nframe = tag->length / (4 * nchan);
        format = SG_AUDIO_F32LE;
        break;

    default:
        sg_logf(SG_LOG_ERROR, "unknown WAVE data format: 0x%04x", afmt);
        goto fmterr;
    }

    sg_riff_destroy(&riff);

    buf->alloc = NULL;
    buf->data = p;
    buf->format = format;
    buf->rate = rate;
    buf->nchan = nchan;
    buf->nframe = nframe;
    return 0;

fmterr:
    sg_error_data(err, "WAVE");
    goto edone;

edone:
    sg_riff_destroy(&riff);
    return -1;
}
