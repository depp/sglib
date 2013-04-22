/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "libpce/binary.h"
#include "libpce/util.h"
#include "sg/audio_ofile.h"
#include "sg/error.h"
#include "sg/file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sg_audio_ofile {
    struct sg_file *fp;
    int samplerate;
    int len;

    unsigned short *tmp;
    size_t tmplen;
};

enum {
    WAV_HEADER_SIZE = 44
};

struct sg_audio_ofile *
sg_audio_ofile_open(const char *path, int samplerate,
                    struct sg_error **err)
{
    struct sg_audio_ofile *afp = NULL;
    struct sg_file *fp = NULL;
    char header[WAV_HEADER_SIZE];
    int r, pos;

    afp = malloc(sizeof(*afp));
    if (!afp) {
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

    afp->fp = fp;
    afp->samplerate = samplerate;
    afp->len = 0;
    afp->tmp = 0;
    afp->tmplen = 0;

    return afp;

cleanup:
    if (afp)
        free(afp);
    if (fp)
        fp->close(fp);
    return NULL;
}

int
sg_audio_ofile_close(struct sg_audio_ofile *afp,
                     struct sg_error **err)
{
    struct sg_file *fp = afp->fp;
    int r, sz, pos, ret;
    int64_t rr;
    char header[WAV_HEADER_SIZE];

    rr = fp->seek(fp, 0, SEEK_SET);
    if (rr < 0) goto error;

    sz = afp->len;
    memcpy(header, "RIFF", 4);
    pce_write_lu32(header + 4, sz * 4 + 36);
    memcpy(header + 8, "WAVE", 4);

    memcpy(header + 12, "fmt ", 4);
    pce_write_lu32(header + 16, 16);
    pce_write_lu16(header + 20, 1);
    pce_write_lu16(header + 22, 2);
    pce_write_lu32(header + 24, afp->samplerate);
    pce_write_lu32(header + 28, afp->samplerate * 4);
    pce_write_lu16(header + 32, 4);
    pce_write_lu16(header + 34, 16);

    memcpy(header + 36, "data", 4);
    pce_write_lu32(header + 40, sz * 4);

    pos = 0;
    while (pos < WAV_HEADER_SIZE) {
        r = fp->write(fp, header, WAV_HEADER_SIZE);
        if (r < 0) goto error;
        pos += r;
    }

    r = fp->close(fp);
    fp = NULL;

    ret = 0;
    goto done;

done:
    if (fp)
        fp->close(fp);
    free(afp->tmp);
    free(afp);
    return ret;

error:
    sg_error_move(err, &fp->err);
    ret = -1;
    goto done;
}

static unsigned short
swap16(unsigned short x)
{
    return (x >> 8) | (x << 8);
}

int
sg_audio_ofile_write(struct sg_audio_ofile *afp,
                     const short *samples, int nframe,
                     struct sg_error **err)
{
    struct sg_file *fp = afp->fp;
    const unsigned char *buf;
    unsigned short *tmp;
    size_t nlen, len, pos;
    int i, r;

    if (PCE_BYTE_ORDER == PCE_BIG_ENDIAN) {
        if (afp->tmplen < (unsigned) nframe) {
            free(afp->tmp);
            afp->tmp = NULL;
            nlen = pce_round_up_pow2(nframe);
            if (nlen > (size_t) -1 / 4)
                goto nomem;
            tmp = malloc(nlen * 4);
            if (!tmp)
                goto nomem;
            afp->tmp = tmp;
            afp->tmplen = nlen;
        } else {
            tmp = afp->tmp;
        }
        for (i = 0; i < nframe; ++i) {
            tmp[2*i+0] = swap16(samples[2*i+0]);
            tmp[2*i+1] = swap16(samples[2*i+1]);
        }
        buf = (unsigned char *) tmp;
    } else {
        buf = (const unsigned char *) samples;
    }

    len = nframe * 4;
    pos = 0;
    while (pos < len) {
        r = fp->write(fp, buf + pos, len - pos);
        if (r < 0)
            goto ferror;
        pos += len;
    }

    return 0;

nomem:
    sg_error_nomem(err);
    return -1;

ferror:
    sg_error_move(err, &fp->err);
    return -1;
}
