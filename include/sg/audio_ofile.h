/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_AUDIO_OFILE_H
#define SG_AUDIO_OFILE_H
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_audio_ofile;

/* Create a new PCM audio file.  */
struct sg_audio_ofile *
sg_audio_ofile_open(const char *path, int samplerate,
                    struct sg_error **err);

int
sg_audio_ofile_close(struct sg_audio_ofile *afp,
                     struct sg_error **err);

int
sg_audio_ofile_write(struct sg_audio_ofile *afp,
                     const short *samples, int nframe,
                     struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
