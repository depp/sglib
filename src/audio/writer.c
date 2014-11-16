/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/binary.h"
#include "sg/util.h"
#include "sg/defs.h"
#include "sg/audio_file.h"
#include "sg/error.h"
#include "sg/file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Information about how the formats get written to a WAVE file: 0x40
   means the format must be byte swapped, 0x80 means the format is
   floating-point.  */

static const unsigned char SG_AUDIO_WRITER_INFO[SG_AUDIO_NFMT] = {
    1,
    2 | 0x40,
    2,
    0,
    0,
    4 | 0x40 | 0x80,
    4        | 0x80
};

static int
sg_audio_writer_fmtsize(sg_audio_format_t format)
{
    return SG_AUDIO_WRITER_INFO[format] & 0x3F;
}

static int
sg_audio_writer_fmtswapped(sg_audio_format_t format)
{
    return (SG_AUDIO_WRITER_INFO[format] & 0x40) != 0;
}

static int
sg_audio_writer_fmtfloat(sg_audio_format_t format)
{
    return (SG_AUDIO_WRITER_INFO[format] & 0x80) != 0;
}

/* Audio writer structure and functions.  */

struct sg_audio_writer {
    struct sg_file *fp;
    sg_audio_format_t format;
    int channelcount;
    int samplerate;
    int len;

    void *tmp;
    size_t tmpalloc;
};

enum {
    WAV_HEADER_SIZE = 44
};

struct sg_audio_writer *
sg_audio_writer_open(const char *path,
                     sg_audio_format_t format,
                     int samplerate, int channelcount,
                     struct sg_error **err)
{
    struct sg_audio_writer *writer = NULL;
    struct sg_file *fp = NULL;
    char header[WAV_HEADER_SIZE];
    int r, pos;

    if (sg_audio_writer_fmtsize(format) == 0) {
        sg_error_invalid(err, __FUNCTION__, "format");
        return NULL;
    }

    writer = malloc(sizeof(*writer));
    if (!writer) {
        sg_error_nomem(err);
        return NULL;
    }

    fp = sg_file_open(path, strlen(path), SG_WRONLY, NULL, err);
    if (!fp) goto cleanup;

    memset(header, 0, WAV_HEADER_SIZE);
    pos = 0;
    while (pos < WAV_HEADER_SIZE) {
        r = fp->write(fp, header, WAV_HEADER_SIZE);
        if (r < 0) {
            sg_error_move(err, &fp->err);
            goto cleanup;
        }
        pos += r;
    }

    writer->fp = fp;
    writer->format = format;
    writer->channelcount = channelcount;
    writer->samplerate = samplerate;
    writer->len = 0;
    writer->tmp = 0;
    writer->tmpalloc = 0;

    return writer;

cleanup:
    if (writer)
        free(writer);
    if (fp)
        fp->close(fp);
    return NULL;
}

int
sg_audio_writer_close(struct sg_audio_writer *writer,
                      struct sg_error **err)
{
    struct sg_file *fp = writer->fp;
    int r, sz, pos, ret, ssize, sfloat, fsize;
    int64_t rr;
    char header[WAV_HEADER_SIZE];

    ssize = sg_audio_writer_fmtsize(writer->format);
    sfloat = sg_audio_writer_fmtfloat(writer->format);
    fsize = writer->channelcount * ssize;

    rr = fp->seek(fp, 0, SEEK_SET);
    if (rr < 0) goto error;

    sz = writer->len;
    memcpy(header, "RIFF", 4);
    sg_write_lu32(header + 4, sz * 4 + 36);
    memcpy(header + 8, "WAVE", 4);

    memcpy(header + 12, "fmt ", 4);
    sg_write_lu32(header + 16, 16);
    sg_write_lu16(header + 20, sfloat ? 3 : 1);
    sg_write_lu16(header + 22, writer->channelcount);
    sg_write_lu32(header + 24, writer->samplerate);
    sg_write_lu32(header + 28, writer->samplerate * fsize);
    sg_write_lu16(header + 32, fsize);
    sg_write_lu16(header + 34, ssize * 8);

    memcpy(header + 36, "data", 4);
    sg_write_lu32(header + 40, sz * 4);

    pos = 0;
    while (pos < WAV_HEADER_SIZE) {
        r = fp->write(fp, header, WAV_HEADER_SIZE);
        if (r < 0) goto error;
        pos += r;
    }

    r = fp->commit(fp);
    if (r)
        goto error;
    ret = 0;
    goto done;

done:
    fp->close(fp);
    free(writer->tmp);
    free(writer);
    return ret;

error:
    sg_error_move(err, &fp->err);
    ret = -1;
    goto done;
}

static void
sg_audio_pcm_swap2(void *SG_RESTRICT out, const void *SG_RESTRICT in,
                   size_t count)
{
    unsigned char *outp = out;
    const unsigned char *inp = in;
    size_t i;
    for (i = 0; i < count; i++) {
        outp[1] = inp[0];
        outp[0] = inp[1];
        inp += 2;
        outp += 2;
    }
}

static void
sg_audio_pcm_swap4(void *SG_RESTRICT out, const void *SG_RESTRICT in,
                   size_t count)
{
    unsigned char *outp = out;
    const unsigned char *inp = in;
    size_t i;
    for (i = 0; i < count; i++) {
        outp[3] = inp[0];
        outp[2] = inp[1];
        outp[1] = inp[2];
        outp[0] = inp[3];
        inp += 4;
        outp += 4;
    }
}

int
sg_audio_writer_write(struct sg_audio_writer *writer,
                      const void *data, int count,
                      struct sg_error **err)
{
    struct sg_file *fp = writer->fp;
    int ssize, sswapped, r;
    size_t nsamp, bsize, pos, nalloc;
    void *tmp;
    const unsigned *buf;

    ssize = sg_audio_writer_fmtsize(writer->format);
    sswapped = sg_audio_writer_fmtswapped(writer->format);
    nsamp = (size_t) writer->channelcount * count;
    bsize = nsamp * ssize;

    if (sswapped) {
        if (writer->tmpalloc < bsize) {
            free(writer->tmp);
            writer->tmp = NULL;
            nalloc = sg_round_up_pow2(bsize);
            if (!nalloc)
                goto nomem;
            tmp = malloc(nalloc);
            if (!tmp)
                goto nomem;
            writer->tmp = tmp;
            writer->tmpalloc = nalloc;
        } else {
            tmp = writer->tmp;
        }
        switch (bsize) {
        case 2:
            sg_audio_pcm_swap2(tmp, data, nsamp);
            break;
        case 4:
            sg_audio_pcm_swap4(tmp, data, nsamp);
            break;
        }
        buf = tmp;
    } else {
        buf = data;
    }

    pos = 0;
    while (pos < bsize) {
        r = fp->write(fp, buf + pos, bsize - pos);
        if (r < 0)
            goto ferror;
        pos += r;
    }

    return 0;

nomem:
    sg_error_nomem(err);
    return -1;

ferror:
    sg_error_move(err, &fp->err);
    return -1;
}
