#include "audio_file.h"
#include "audio_wav.h"
#include "binary.h"
#include "error.h"
#include "log.h"
#include "sgendian.h"
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
    rlength = read_lu32(p + 4);
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
        t.length = read_lu32(p + pos + 4);
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
sg_audio_file_checkwav(const void *data, size_t length)
{
    const unsigned char *p = data;
    if (length < 12)
        return 0;
    if (!memcmp(p, "RIFF", 4) && !memcmp(p + 8, "WAVE", 4))
        return 1;
    return 0;
}

static struct sg_logger *
sg_audio_wav_logger(void)
{
    return sg_logger_get("audio");
}

int
sg_audio_file_loadwav(struct sg_audio_pcm *pcm,
                      const void *data, size_t length,
                      struct sg_error **err)
{
    struct sg_riff riff;
    struct sg_riff_tag *tag;
    int r;
    uint16_t afmt, nchan, blkalign, sampbits;
    uint32_t rate, nframe;
    const unsigned char *p;

    r = sg_riff_parse(&riff, data, length, err);
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
    afmt = read_lu16(p + 0);
    nchan = read_lu16(p + 2);
    rate = read_lu32(p + 4);
    blkalign = read_lu16(p + 12);
    sampbits = read_lu16(p + 14);
    if (rate < SG_AUDIO_MINRATE || rate > SG_AUDIO_MAXRATE) {
        sg_logf(sg_audio_wav_logger(), LOG_ERROR,
                "WAVE sample rate too extreme (%u Hz)", rate);
        goto fmterr;
    }
    if (nchan != 1 && nchan != 2) {
        sg_logf(sg_audio_wav_logger(), LOG_ERROR,
                "WAVE has unsupported number of channels (%u)", nchan);
        goto fmterr;
    }

    /* Read WAVE data */
    tag = sg_riff_get(&riff, "data");
    if (!tag)
        goto fmterr;
    p = tag->data;

    switch (afmt) {
    case SG_WAVE_PCM:
        sg_logs(sg_audio_wav_logger(), LOG_ERROR,
                "PCM data unsupported, yet...");
        goto fmterr;

    case SG_WAVE_FLOAT:
        if (sampbits != 32) {
            sg_logs(sg_audio_wav_logger(), LOG_ERROR,
                    "WAVE float bits != 32");
            goto fmterr;
        }
        nframe = tag->length / (4 * nchan);
        pcm->nframe = nframe;
        pcm->nchan = nchan;
        pcm->rate = rate;
        r = sg_audio_pcm_alloc(pcm);
        if (r) goto memerr;
        sg_audio_copy_float32(pcm->data, p, nframe * nchan,
                              BYTE_ORDER != LITTLE_ENDIAN);
        break;

    default:
        sg_logf(sg_audio_wav_logger(), LOG_ERROR,
                "unknown WAVE data format: 0x%04x", afmt);
        goto fmterr;
    }

    sg_riff_destroy(&riff);
    return 0;

fmterr:
    sg_error_data(err, "WAVE");
    goto edone;

memerr:
    sg_error_nomem(err);
    goto edone;

edone:
    sg_riff_destroy(&riff);
    return -1;
}
