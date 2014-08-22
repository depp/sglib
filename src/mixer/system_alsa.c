/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#define _XOPEN_SOURCE 600

#include "../core/clock_impl.h"
#include "mixer.h"
#include "sg/byteorder.h"
#include "sg/cvar.h"
#include "sg/entry.h"
#include "sg/error.h"
/* #include "sg/event.h" */
#include "sg/log.h"
#include <alloca.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

struct sg_mixer_alsa {
    struct sg_logger *logger;

    sg_atomic_t stop_requested;

    /* Mutex so that only one thread can access the audio state.  */
    pthread_mutex_t mutex;
    /* Condition variable to signal state changes.  */
    pthread_cond_t cond;

    /* Current audio system state.  */
    int is_running;

    /* Requested audio device name.  */
    const char *req_name;
    /* Requested sample rate.  */
    int req_rate;
    /* Requested buffer size.  */
    int req_bufsize;

    /* The ALSA audio stream.  */
    snd_pcm_t *pcm;
    /* The buffer for interleaved data.  */
    float *buffer;
    /* The sample rate.  */
    int rate;
    /* The size of ALSA's audio buffer, NOT the size of the buffer
       used in the game, which ALSA calls the period size.  */
    int bufsize;
    /* The number of frames to fill for each request.  */
    int periodsize;
};

static struct sg_mixer_alsa sg_mixer_alsa;

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
    /* union sg_event event; */
    struct sg_mixer_mixdowniface *mp = ptr;
    struct sg_mixer_alsa *alsa = &sg_mixer_alsa;
    snd_pcm_t *pcm = alsa->pcm;
    unsigned periodsize = alsa->periodsize, bufsize = alsa->bufsize,
        rate = alsa->rate, time, pos, dx, dt, nsec;
    float *buf = alsa->buffer;
    snd_pcm_sframes_t r;
    snd_pcm_uframes_t nframes;
    struct timespec ts;
    snd_pcm_status_t *status;
    int rr, rcount;
    snd_pcm_status_alloca(&status);

    (void) ptr;

    /*
    event.audioinit.type = SG_EVENT_AUDIO_INIT;
    event.audioinit.rate = rate;
    event.audioinit.buffersize = periodsize;
    sg_game_event(&event);
    */

    while (!sg_atomic_get(&alsa->stop_requested)) {
        snd_pcm_wait(pcm, 1000);
        r = snd_pcm_status(pcm, status);
        if (r < 0) {
            sg_logf(alsa->logger, SG_LOG_INFO,
                    "snd_pcm_status: %s", snd_strerror(r));
            break;
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

        rcount = sg_mixer_mixdown_process(mp, time);
        if (rcount == 0)
            break;
        sg_mixer_mixdown_get_f32(mp, buf);
        pos = 0;
        while (pos < periodsize) {
            r = snd_pcm_writei(pcm, buf + 2 * pos, periodsize - pos);
            if (r < 0) {
                switch (-r) {
                case EAGAIN:
                    break;

                case EPIPE:
                prepare:
                    r = snd_pcm_prepare(pcm);
                    if (r < 0) {
                        sg_logf(alsa->logger, SG_LOG_INFO,
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
                    sg_logf(alsa->logger, SG_LOG_ERROR,
                            "snd_pcm_writei: %s", snd_strerror(r));
                    goto done;
                }
            } else {
                pos += r;
            }
        }
    }

done:

    /*
    event.type = SG_EVENT_AUDIO_TERM;
    sg_game_event(&event);
    */

    sg_mixer_mixdown_free(mp);

    rr = pthread_mutex_lock(&alsa->mutex);
    if (rr) abort();
    snd_pcm_close(alsa->pcm);
    alsa->is_running = 0;
    alsa->pcm = NULL;
    alsa->rate = 0;
    alsa->bufsize = 0;
    alsa->periodsize = 0;
    rr = pthread_mutex_unlock(&alsa->mutex);
    if (rr) abort();
    rr = pthread_cond_broadcast(&alsa->cond);
    if (rr) abort();

    return NULL;
}

void
sg_mixer_system_init(void)
{
    struct sg_mixer_alsa *alsa = &sg_mixer_alsa;
    int r;
    pthread_mutexattr_t mattr;

    alsa->logger = sg_logger_get("audio");
    sg_atomic_set(&alsa->stop_requested, 0);

    r = pthread_mutexattr_init(&mattr);
    if (r) abort();
    r = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
    if (r) abort();
    r = pthread_mutex_init(&alsa->mutex, &mattr);
    if (r) abort();
    r = pthread_mutexattr_destroy(&mattr);
    if (r) abort();

    r = pthread_cond_init(&alsa->cond, NULL);
    if (r) abort();

    if (!sg_cvar_gets("audio", "alsadevice", &alsa->req_name))
        alsa->req_name = "default";
    if (!sg_cvar_geti("audio", "rate", &alsa->req_rate))
        alsa->req_rate = 48000;
    if (!sg_cvar_geti("audio", "bufsize", &alsa->req_bufsize))
        alsa->req_bufsize = 1024;

    if (alsa->req_rate < 8000)
        alsa->req_rate = 8000;
    if (alsa->req_rate > 192000)
        alsa->req_rate = 192000;

    if (alsa->req_bufsize < 32)
        alsa->req_bufsize = 32;
    if (alsa->req_bufsize > 4096)
        alsa->req_bufsize = 4096;
}

void
sg_mixer_start(void)
{
    struct sg_mixer_mixdowniface *mp = NULL;
    struct sg_mixer_alsa *alsa = &sg_mixer_alsa;
    snd_pcm_hw_params_t *hwparms = NULL;
    snd_pcm_sw_params_t *swparms = NULL;
    pthread_t thread;
    pthread_attr_t threadattr;
    const char *why;
    int r;
    unsigned rate;
    snd_pcm_uframes_t bufsize, periodsize, avail_min;
    struct sg_error *err = NULL;

    r = pthread_mutex_lock(&alsa->mutex);
    if (r) abort();
    if (alsa->is_running) {
        r = pthread_mutex_unlock(&alsa->mutex);
        if (r) abort();
        sg_logs(alsa->logger, SG_LOG_INFO,
                "audio system already running");
        return;
    }

    sg_atomic_set(&alsa->stop_requested, 0);

    /* Open ALSA PCM connection */
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
        ((SG_BYTE_ORDER == SG_LITTLE_ENDIAN) ?
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
    sg_logf(alsa->logger, SG_LOG_INFO,
            "audio sample rate: %u", rate);
    if (rate > 200000 || rate < 4000) {
        sg_logf(alsa->logger, SG_LOG_ERROR,
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
    sg_logf(alsa->logger, SG_LOG_INFO,
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

    r = snd_pcm_sw_params_get_avail_min(swparms, &avail_min);
    if (r < 0) {
        why = "could not get avail_min";
        goto alsa_error;
    }
    sg_logf(alsa->logger, SG_LOG_INFO,
            "audio buffer threshold: %u", (unsigned) avail_min);

    /* ==================== */

    alsa->buffer = malloc(sizeof(float) * periodsize * 2);
    if (alsa->buffer == NULL) {
        sg_error_nomem(&err);
        sg_logerrs(alsa->logger, SG_LOG_ERROR, err,
                   "failed to create buffer");
        goto cleanup;
    }

    mp = sg_mixer_mixdown_new_live(rate, periodsize, &err);
    if (!mp) {
        sg_logerrs(alsa->logger, SG_LOG_ERROR, err,
                   "failed to create mixdown");
        goto cleanup;
    }

    /* Spawn audio thread */
    r = pthread_attr_init(&threadattr);
    if (r) goto pthread_error;
    r = pthread_attr_setdetachstate(&threadattr, PTHREAD_CREATE_DETACHED);
    if (r) goto pthread_error;
    r = pthread_create(&thread, &threadattr, sg_audio_loop, mp);
    if (r) goto pthread_error;
    pthread_attr_destroy(&threadattr);

    alsa->is_running = 1;
    r = pthread_mutex_unlock(&alsa->mutex);
    if (r) abort();

    return;

alsa_error:
    sg_logf(alsa->logger, SG_LOG_ERROR,
            "%s: %s", why, snd_strerror(r));
    goto cleanup;

pthread_error:
    sg_logs(alsa->logger, SG_LOG_ERROR, strerror(r));
    goto cleanup;

cleanup:
    sg_error_clear(&err);
    free(alsa->buffer);
    if (alsa->pcm)
        snd_pcm_close(alsa->pcm);
    if (mp)
        sg_mixer_mixdown_free(mp);
    r = pthread_mutex_unlock(&alsa->mutex);
    if (r) abort();
}

void
sg_mixer_stop(void)
{
    struct sg_mixer_alsa *alsa = &sg_mixer_alsa;
    int r;
    r = pthread_mutex_lock(&alsa->mutex);
    if (r) abort();
    if (alsa->is_running) {
        sg_atomic_set(&alsa->stop_requested, 1);
        while (alsa->is_running) {
            r = pthread_cond_wait(&alsa->cond, &alsa->mutex);
            if (r) abort();
        }
    }
    r = pthread_mutex_unlock(&alsa->mutex);
    if (r) abort();
}
