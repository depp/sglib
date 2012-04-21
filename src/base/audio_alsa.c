#define _XOPEN_SOURCE 600

#include "audio.h"
#include "clock.h"
#include "cvar.h"
#include "error.h"
#include "log.h"
#include "sgendian.h"
#include <alloca.h>
#include <alsa/asoundlib.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct sg_audio_alsa {
    const char *name;
    int rate;
    int bufsize;

    snd_pcm_t *pcm;
    struct sg_audio_system *sys;
    float *buffer;
    int realrate;
    int periodsize;
};

static struct sg_logger *sg_audio_alsa_logger;

static void
sg_audio_alsa_getinfo(struct sg_audio_alsa *alsa)
{
    if (!sg_cvar_gets("audio", "alsadevice", &alsa->name))
        alsa->name = "default";
    if (!sg_cvar_geti("audio", "rate", &alsa->rate))
        alsa->rate = 48000;
    assert(alsa->rate > 0);
    if (!sg_cvar_geti("audio", "bufsize", &alsa->bufsize))
        alsa->bufsize = 1024;
    assert(alsa->bufsize > 0);
}


static void *
sg_audio_loop(void *ptr)
{
    struct sg_audio_alsa *alsa = ptr;
    struct sg_audio_system *sys = alsa->sys;
    snd_pcm_t *pcm = alsa->pcm;
    float *buf = alsa->buffer;
    unsigned time, periodsize = alsa->periodsize, pos;
    snd_pcm_sframes_t r;

    while (1) {
        time = sg_clock_get();
        sg_audio_system_pull(sys, time, buf, periodsize);
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
sg_audio_initsys(void)
{
    struct sg_error *err = NULL;
    struct sg_audio_alsa *alsa;
    snd_pcm_hw_params_t *hwparms = NULL;
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
    alsa->sys = NULL;

    /* Open ALSA PCM connection */
    sg_audio_alsa_getinfo(alsa);
    r = snd_pcm_open(
        &alsa->pcm,
        alsa->name,
        SND_PCM_STREAM_PLAYBACK,
        0);
    if (r < 0) {
        why = "could not open audio device";
        goto alsa_error;
    }

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
        ((BYTE_ORDER == LITTLE_ENDIAN) ?
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

    rate = alsa->rate;
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
    alsa->realrate = rate;

    bufsize = alsa->bufsize;
    r = snd_pcm_hw_params_set_buffer_size_near(
        alsa->pcm, hwparms, &bufsize);
    if (r < 0) {
        why = "could not set buffer size";
        goto alsa_error;
    }
    sg_logf(sg_audio_alsa_logger, LOG_INFO,
            "audio buffer size: %lu", bufsize);

    r = snd_pcm_hw_params_get_period_size(
        hwparms, &periodsize, 0);
    if (r < 0) {
        why = "could not get period size";
        goto cleanup;
    }
    sg_logf(sg_audio_alsa_logger, LOG_INFO,
            "audio period size: %lu", periodsize);
    assert(periodsize < INT_MAX);
    alsa->periodsize = periodsize;

    r = snd_pcm_hw_params(alsa->pcm, hwparms);
    if (r < 0) {
        why = "could not set hardware parameters";
        goto alsa_error;
    }

    alsa->sys = sg_audio_system_new(rate, &err);
    if (!alsa->sys) {
        sg_logerrs(sg_audio_alsa_logger, LOG_ERROR, err,
                   "could not create audio subsystem");
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
        if (alsa->sys)
            sg_audio_system_free(alsa->sys);
        if (alsa)
            free(alsa);
    }
    return;

nomem:
    sg_logs(sg_audio_alsa_logger, LOG_ERROR,
            "out of memory");
    abort();
}
