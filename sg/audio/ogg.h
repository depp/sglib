/* Copyright 2013 Dietrich Epp <depp@zdome.net> */
#ifndef SG_AUDIO_OGG_H
#define SG_AUDIO_OGG_H
#include <ogg/ogg.h>
struct sg_error;
struct sg_audio_pcm;

/* ========== Opus ========== */

void *
sg_opus_decoder_new(struct sg_error **err);

void
sg_opus_decoder_free(void *obj);

int
sg_opus_decoder_packet(void *obj, ogg_packet *op,
                       struct sg_error **err);

int
sg_opus_decoder_read(void *obj, struct sg_audio_pcm *pcm,
                     struct sg_error **err);

/* ========== Vorbis ========== */

void *
sg_vorbis_decoder_new(struct sg_error **err);

void
sg_vorbis_decoder_free(void *obj);

int
sg_vorbis_decoder_packet(void *obj, ogg_packet *op,
                         struct sg_error **err);

int
sg_vorbis_decoder_read(void *obj, struct sg_audio_pcm *pcm,
                       struct sg_error **err);

#endif
