#include "audio_file.h"
#include "audio_fileprivate.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>

static void
sg_audio_file_resample_1(short *restrict dest, int dlen, int drate,
                         const short *restrict src, int slen, int srate)
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
sg_audio_file_resample_2(short *restrict dest, int dlen, int drate,
                         const short *restrict src, int slen, int srate)
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
sg_audio_file_resample(struct sg_audio_file *fp, int rate,
                       struct sg_error **err)
{
    unsigned nlen, chan;
    short *ndata;
    int orate;

    orate = fp->rate;
    chan = (fp->flags & SG_AUDIOFILE_STEREO) ? 2 : 1;
    nlen = (int) (fp->nframe * ((double) rate / orate));
    if (nlen < 1)
        nlen = 1;

    ndata = malloc(sizeof(short) * chan * nlen);
    if (!ndata) {
        sg_error_nomem(err);
        return -1;
    }

    if (chan == 1)
        sg_audio_file_resample_1(ndata, nlen, rate,
                                 fp->data, fp->nframe, fp->rate);
    else
        sg_audio_file_resample_2(ndata, nlen, rate,
                                 fp->data, fp->nframe, fp->rate);

    free(fp->data);
    /* FIXME: FIXMEATOMIC */
    fp->flags |= SG_AUDIOFILE_RESAMPLED;
    fp->nframe = nlen;
    fp->rate = rate;
    fp->data = ndata;
    return 0;
}
