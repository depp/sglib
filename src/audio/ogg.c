/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "config.h"
#include "ogg.h"
#include "sg/audio_pcm.h"
#include "sg/error.h"
#include "sg/log.h"
#include <stdlib.h>
#include <string.h>

#define SG_OGG_CHUNKSIZE (8*1024)
#define SG_OGG_MAXSTREAM 8

/* States for logical bitstreams */
enum {
    SG_OGG_HEADER,
    SG_OGG_IGNORE,
    SG_OGG_DECODE
};

struct sg_ogg_stream {
    int serial;
    int state;
    ogg_stream_state os;
};

int
sg_audio_pcm_loadogg(struct sg_audio_pcm **buf, size_t *bufcount,
                     const void *data, size_t len,
                     struct sg_error **err)
{
    ogg_sync_state oy;
    ogg_page og;
    ogg_packet op;
    char *obuf;
    size_t pos, amt;
    int serial, state, streamcount = 0, i, r;
    struct sg_ogg_stream stream[SG_OGG_MAXSTREAM];
    const char *msg;

    struct sg_audio_pcm *pcm = NULL, *npcm;
    size_t pcmcount = 0, pcmalloc = 0, k;

    void *decoder = NULL;
    void (*decoder_free)(void *) = NULL;
    int (*decoder_packet)(void *, ogg_packet *, struct sg_error **) = NULL;
    int (*decoder_read)(void *, struct sg_audio_pcm *,
                        struct sg_error **) = NULL;

    /* always returs 0 */
    ogg_sync_init(&oy);

    pos = 0;
    while (1) {
        /* This is lame, but we copy the data we already read into the
           Ogg structure one chunk at a time.  */
        while (ogg_sync_pageout(&oy, &og) != 1) {
            amt = SG_OGG_CHUNKSIZE;
            if (amt > len - pos)
                amt = len - pos;
            if (!amt)
                goto ogg_eof;
            obuf = ogg_sync_buffer(&oy, amt);
            memcpy(obuf, (const char *) data + pos, amt);
            pos += amt;
            r = ogg_sync_wrote(&oy, amt);
            if (r) {
                msg = "ogg_sync_wrote";
                goto ogg_error;
            }
        }

        serial = ogg_page_serialno(&og);
        for (i = 0; i < streamcount; i++)
            if (serial == stream[i].serial)
                break;

        if (i == streamcount) {
            if (!ogg_page_bos(&og)) {
                msg = "expected BOS";
                goto ogg_error;
            }
            if (streamcount >= SG_OGG_MAXSTREAM) {
                msg = "too many streams";
                goto ogg_error;
            }
            streamcount++;
            stream[i].serial = serial;
            if (!decoder) {
                stream[i].state = state = SG_OGG_HEADER;
                r = ogg_stream_init(&stream[i].os, serial);
                if (r) {
                    msg = "ogg_stream_init";
                    goto ogg_error;
                }
            } else {
                stream[i].state = state = SG_OGG_IGNORE;
            }
        } else {
            if (ogg_page_bos(&og)) {
                msg = "unexpected BOS";
                goto ogg_error;
            }
            state = stream[i].state;
        }

        if (state == SG_OGG_IGNORE)
            goto finish_page;

        r = ogg_stream_pagein(&stream[i].os, &og);
        if (r != 0) {
            msg = "ogg_stream_pagein";
            goto ogg_error;
        }

        r = ogg_stream_packetout(&stream[i].os, &op);
        if (r != 1)
            goto finish_page;

        if (state == SG_OGG_HEADER) {
            if (op.bytes >= 7 &&
                !memcmp(op.packet, "\1vorbis", 7)) {
#if defined(ENABLE_VORBIS)
                stream[i].state = SG_OGG_DECODE;
                decoder = sg_vorbis_decoder_new(err);
                decoder_free = sg_vorbis_decoder_free;
                decoder_packet = sg_vorbis_decoder_packet;
                decoder_read = sg_vorbis_decoder_read;
#else
                msg = "Vorbis is not supported";
                goto ogg_error;
#endif
            } else if (op.bytes >= 8 &&
                       !memcmp(op.packet, "OpusHead", 8)) {
#if defined(ENABLE_OPUS)
                stream[i].state = SG_OGG_DECODE;
                decoder = sg_opus_decoder_new(err);
                decoder_free = sg_opus_decoder_free;
                decoder_packet = sg_opus_decoder_packet;
                decoder_read = sg_opus_decoder_read;
#else
                msg = "Opus is not supported";
                goto ogg_error;
#endif

            } else {
                ogg_stream_clear(&stream[i].os);
                stream[i].state = state = SG_OGG_IGNORE;
                goto finish_page;
            }
        }

        while (1) {
            r = decoder_packet(decoder, &op, err);
            if (r) {
                r = -1;
                goto done;
            }
            r = ogg_stream_packetout(&stream[i].os, &op);
            if (r != 1)
                break;
        }
        if (r != 0) {
            msg = "ogg_stream_packetout";
            goto ogg_error;
        }

    finish_page:
        if (ogg_page_eos(&og)) {
            switch (state) {
            case SG_OGG_HEADER:
                ogg_stream_clear(&stream[i].os);
                break;

            case SG_OGG_IGNORE:
                break;

            case SG_OGG_DECODE:
                if (pcmcount >= pcmalloc) {
                    pcmalloc = pcmalloc ? pcmalloc * 2 : 1;
                    npcm = realloc(pcm, sizeof(*pcm) * pcmalloc);
                    if (!npcm) {
                        sg_error_nomem(err);
                        r = -1;
                        goto done;
                    }
                    pcm = npcm;
                }
                r = decoder_read(decoder, &pcm[pcmcount], err);
                if (r)
                    goto done;
                pcmcount++;
                decoder_free(decoder);
                decoder = NULL;
                ogg_stream_clear(&stream[i].os);
                break;
            }
            streamcount--;
            stream[i] = stream[streamcount];
        }
    }

ogg_eof:
    if (streamcount) {
        msg = "unexpected EOF";
        goto ogg_error;
    }
    r = 0;
    goto done;

ogg_error:
    sg_logf(sg_logger_get("audio"), LOG_ERROR,
            "Ogg error: %s", msg);
    sg_error_data(err, "ogg");
    r = -1;
    goto done;

done:
    ogg_sync_clear(&oy);
    for (i = 0; i < streamcount; i++) {
        if (stream[i].state != SG_OGG_IGNORE)
            ogg_stream_clear(&stream[i].os);
    }
    if (decoder)
        decoder_free(decoder);
    if (!r) {
        if (!pcmcount) {
            sg_logs(sg_logger_get("audio"), LOG_ERROR,
                    "No streams with known codecs were found");
            sg_error_data(err, "ogg");
            return -1;
        }
        *buf = pcm;
        *bufcount = pcmcount;
        return 0;
    } else {
        for (k = 0; k < pcmcount; k++)
            sg_audio_pcm_destroy(&pcm[k]);
        return -1;
    }
}
