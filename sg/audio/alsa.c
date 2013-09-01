/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#define _XOPEN_SOURCE 600

#include "../core/clock_impl.h"
#include "libpce/byteorder.h"
#include "sg/audio_mixdown.h"
#include "sg/audio_system.h"
#include "sg/clock.h"
#include "sg/cvar.h"
#include "sg/error.h"
#include "sg/log.h"
#include <alloca.h>
#include <alsa/asoundlib.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct sg_audio_alsa {
    const char *req_name;
    int req_rate;
    int req_bufsize;

    snd_pcm_t *pcm;
    struct sg_audio_mixdown *mix;
    float *buffer;
    int rate;
    int bufsize;
    int periodsize;
};

static struct sg_logger *sg_audio_alsa_logger;

static void
sg_audio_alsa_getinfo(struct sg_audio_alsa *alsa)
{
    if (!sg_cvar_gets("audio", "alsadevice", &alsa->req_name))
        alsa->req_name = "default";
    if (!sg_cvar_geti("audio", "rate", &alsa->req_rate))
        alsa->req_rate = 48000;
    assert(alsa->req_rate > 0);
    if (!sg_cvar_geti("audio", "bufsize", &alsa->req_bufsize))
        alsa->req_bufsize = 1024;
    assert(alsa->req_bufsize > 0);
}

/*
  ALSA timestamps:

  - Retrieve the status with 'snd_pcm_status'
  - Get "now" timestamp with 'snd_pcm_status_get_htstamp'
  - Get number of frames available with 'snd_pcm_status_get_avail'

  The "now" timestamp is the timestamp of the frame currently being
  read, so to get the timestamp of the frame currently being written
  we have to add the length of sound currently in the buffer.  This is
  calculated by subtracting available frames from the buffer size, and
  then dividing by the sample rate.

  Tests indicate that the resulting timestamps are relatively stable,
  and unaffected by any additional calls to 'usleep' in the callback
  function.

  The ALSA-supplied timestamps are not as useful as they could be.
  They use either the monotonic clock or the realtime clock, but there
  is no good way to tell which.  Since we just woke up, we simply make
  a separate call to get the monotonic clock, and use this timestamp
  to calculate the game tick corresponding to the current write
  position.

  So we end up calculating the time stamp of the first sample in the
  write buffer, and then platform-independent code will manage latency
  and jitter.
*/
static void *
sg_audio_loop(void *ptr)
{
    struct sg_audio_alsa *alsa = ptr;
    struct sg_audio_mixdown *mix = alsa->mix;
    snd_pcm_t *pcm = alsa->pcm;
    float *buf = alsa->buffer;
    unsigned periodsize = alsa->periodsize, bufsize = alsa->bufsize,
        rate = alsa->rate, time, pos, dx, dt, nsec;
    snd_pcm_sframes_t r;
    snd_pcm_uframes_t nframes;
    struct timespec ts;
    snd_pcm_status_t *status;
    snd_pcm_status_alloca(&status);

    while (1) {
        snd_pcm_wait(pcm, 1000);

        r = snd_pcm_status(pcm, status);
        if (r < 0) {
            sg_logf(sg_audio_alsa_logger, LOG_INFO,
                    "snd_pcm_status: %s", snd_strerror(r));
            goto done;
        }
        nframes = snd_pcm_status_get_avail(status);
        clock_gettime(CLOCK_MONOTONIC, &ts);

        dx = bufsize - nframes;
        /* This will overflow if latency is more than a couple
           seconds.  If latency is that bad, we don't care.  */
        dt = (unsigned) ((1000000000ULL * dx) / rate);
        nsec = ts.tv_nsec + dt;
        ts.tv_sec += nsec / 1000000000;
        ts.tv_nsec = nsec % 1000000000;
        time = sg_clock_convert(&ts);

        sg_audio_mixdown_read(mix, time, buf);
        pos = 0;
        while (pos < periodsize) {
            r = snd_pcm_writei(pcm, buf + pos, periodsize - pos);
            if (r < 0) {
                switch (-r) {
                case EAGAIN:
                    break;

                case EPIPE:
                prepare:
                    r = snd_pcm_prepare(pcm);
                    if (r < 0) {
                        sg_logf(sg_audio_alsa_logger, LOG_INFO,
                                "snd_pcm_prepare: %s", snd_strerror(r));
                    }
                    break;

                case ESTRPIPE:
                    while (1) {
                        r = snd_pcm_resume(pcm);
                        if (r != -EAGAIN)
                            break;
                        sleep(1);
                    }
                    if (r < 0)
                        goto prepare;
                    break;

                default:
                    sg_logf(sg_audio_alsa_logger, LOG_ERROR,
                            "snd_pcm_writei: %s", snd_strerror(r));
                    goto done;
                }
            } else {
                pos += r;
            }
        }
    }
done:
    /* FIXME LEAK */
    return NULL;
}

void
sg_audio_sys_pstart(void)
{
    struct sg_error *err = NULL;
    struct sg_audio_alsa *alsa;
    snd_pcm_hw_params_t *hwparms = NULL;
    snd_pcm_sw_params_t *swparms = NULL;
    pthread_t thread;
    pthread_attr_t threadattr;
    const char *why;
    int r;
    unsigned rate;
    snd_pcm_uframes_t bufsize, periodsize;

    sg_audio_alsa_logger = sg_logger_get("audio");

    /* Get configuration, initialize data structures */
    alsa = malloc(sizeof(*alsa));
    if (!alsa)
        goto nomem;
    alsa->pcm = NULL;
    alsa->mix = NULL;

    /* Open ALSA PCM connection */
    sg_audio_alsa_getinfo(alsa);
    r = snd_pcm_open(
        &alsa->pcm,
        alsa->req_name,
        SND_PCM_STREAM_PLAYBACK,
        0);
    if (r < 0) {
        why = "could not open audio device";
        goto alsa_error;
    }

    /* ==================== */

    snd_pcm_hw_params_alloca(&hwparms);
    r = snd_pcm_hw_params_any(alsa->pcm, hwparms);
    if (r < 0) {
        why = "could not get hardware parameters";
        goto alsa_error;
    }

    r = snd_pcm_hw_params_set_rate_resample(
        alsa->pcm, hwparms, 0);
    if (r < 0) {
        why = "could not disable resampling";
        goto alsa_error;
    }

    r = snd_pcm_hw_params_set_access(
        alsa->pcm, hwparms,
        SND_PCM_ACCESS_RW_INTERLEAVED);
    if (r < 0) {
        why = "could not set access mode";
        goto alsa_error;
    }

    r = snd_pcm_hw_params_set_format(
        alsa->pcm, hwparms,
        ((PCE_BYTE_ORDER == PCE_LITTLE_ENDIAN) ?
         SND_PCM_FORMAT_FLOAT_LE : SND_PCM_FORMAT_FLOAT_BE));
    if (r < 0) {
        why = "could not set sample format";
        goto alsa_error;
    }

    r = snd_pcm_hw_params_set_channels(
        alsa->pcm, hwparms, 2);
    if (r < 0) {
        why = "could not set channel count";
        goto alsa_error;
    }

    rate = alsa->req_rate;
    r = snd_pcm_hw_params_set_rate_near(
        alsa->pcm, hwparms, &rate, 0);
    if (r < 0) {
        why = "could not set sample rate";
        goto alsa_error;
    }
    sg_logf(sg_audio_alsa_logger, LOG_INFO,
            "audio sample rate: %u", rate);
    if (rate > 200000 || rate < 4000) {
        sg_logf(sg_audio_alsa_logger, LOG_ERROR,
                "audio sample rate too extreme: %u", rate);
        goto cleanup;
    }
    alsa->rate = rate;

    bufsize = alsa->req_bufsize;
    r = snd_pcm_hw_params_set_buffer_size_near(
        alsa->pcm, hwparms, &bufsize);
    if (r < 0) {
        why = "could not set buffer size";
        goto alsa_error;
    }
    sg_logf(sg_audio_alsa_logger, LOG_INFO,
            "audio buffer size: %lu", bufsize);
    alsa->bufsize = bufsize;
    periodsize = (bufsize + 1) >> 1;
    alsa->periodsize = periodsize;

    r = snd_pcm_hw_params(alsa->pcm, hwparms);
    if (r < 0) {
        why = "could not set hardware parameters";
        goto alsa_error;
    }

    /* ==================== */

    snd_pcm_sw_params_alloca(&swparms);
    r = snd_pcm_sw_params_current(alsa->pcm, swparms);
    if (r < 0) {
        why = "could not get software parameters";
        goto alsa_error;
    }

    snd_pcm_uframes_t avail_min;
    r = snd_pcm_sw_params_get_avail_min(swparms, &avail_min);
    if (r < 0) {
        why = "could not get avail_min";
        goto alsa_error;
    }
    sg_logf(sg_audio_alsa_logger, LOG_INFO,
            "audio buffer threshold: %u", (unsigned) avail_min);

    /* ==================== */

    alsa->mix = sg_audio_mixdown_new(rate, periodsize, &err);
    if (!alsa->mix) {
        sg_logerrs(sg_audio_alsa_logger, LOG_ERROR, err,
                   "could not create audio mixdown");
        sg_error_clear(&err);
        goto cleanup;
    }

    alsa->buffer = malloc(sizeof(float) * periodsize * 2);
    if (!alsa->buffer)
        goto nomem;

    /* Spawn audio thread */
    r = pthread_attr_init(&threadattr);
    if (r) goto pthread_error;
    r = pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_DETACHED);
    if (r) goto pthread_error;
    r = pthread_create(&thread, &threadattr, sg_audio_loop, alsa);
    if (r) goto pthread_error;
    pthread_attr_destroy(&threadattr);

    return;

alsa_error:
    sg_logf(sg_audio_alsa_logger, LOG_ERROR,
            "%s: %s", why, snd_strerror(r));
    goto cleanup;

pthread_error:
    sg_logs(sg_audio_alsa_logger, LOG_ERROR, strerror(r));
    goto cleanup;

cleanup:
    if (alsa) {
        if (alsa->pcm)
            snd_pcm_close(alsa->pcm);
        if (alsa->mix)
            sg_audio_mixdown_free(alsa->mix);
        if (alsa)
            free(alsa);
    }
    return;

nomem:
    sg_logs(sg_audio_alsa_logger, LOG_ERROR,
            "out of memory");
    abort();
}

void
sg_audio_sys_pstop(void)
{ }
