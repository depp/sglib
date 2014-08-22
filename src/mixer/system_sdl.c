/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "mixer.h"
#include "sg/clock.h"
#include "sg/cvar.h"
#include "sg/error.h"
#include "sg/log.h"
#include "SDL.h"

struct sg_mixer_sdl {
    struct sg_logger *logger;

    /* The system was successfully initialized.  */
    int is_initted;

    /* The active audio device, or 0 if audio is not running.  */
    SDL_AudioDeviceID dev;

    /* Requested sample rate.  */
    int req_rate;
    /* Requested buffer size.  */
    int req_bufsize;

    /* The mixdown.  */
    struct sg_mixer_mixdowniface *mixdown;
    /* The sample rate.  */
    int rate;
    /* The size of the buffer, in frames.  */
    int bufsize;
};

static struct sg_mixer_sdl sg_mixer_sdl;

static void
sg_mixer_sdl_error(const char *msg)
{
    sg_logf(sg_mixer_sdl.logger, SG_LOG_ERROR,
            "SDL audio: %s: %s", msg, SDL_GetError());
}

void
sg_mixer_system_init(void)
{
    struct sg_mixer_sdl *ap = &sg_mixer_sdl;

    ap->logger = sg_logger_get("audio");

    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        sg_mixer_sdl_error("could not initialize SDL subsystem");
        return;
    }

    if (!sg_cvar_geti("audio", "rate", &ap->req_rate))
        ap->req_rate = 48000;
    if (!sg_cvar_geti("audio", "bufsize", &ap->req_bufsize))
        ap->req_bufsize = 1024;
    ap->is_initted = 1;
}

static void
sg_mixer_sdl_callback(void *userdata, Uint8 *stream, int len)
{
    struct sg_mixer_sdl *ap = &sg_mixer_sdl;
    struct sg_mixer_mixdowniface *mp = ap->mixdown;
    unsigned time;

    (void) userdata;

    time = sg_clock_get();
    sg_mixer_mixdown_process(mp, time);
    if (len != sizeof(float) * ap->bufsize * 2) {
        if (ap->bufsize)
            sg_logf(ap->logger, SG_LOG_ERROR,
                    "unexpected audio buffer size: %d", len);
        ap->bufsize = 0;
        memset(stream, 0, len);
    } else {
        sg_mixer_mixdown_get_f32(mp, (float *) stream);
    }
}

void
sg_mixer_start(void)
{
    struct sg_mixer_sdl *ap = &sg_mixer_sdl;
    struct sg_mixer_mixdowniface *mp;
    SDL_AudioSpec req, spec;
    SDL_AudioDeviceID dev;
    struct sg_error *err = NULL;

    if (!ap->is_initted)
        return;

    memset(&req, 0, sizeof(req));
    req.freq = ap->req_rate;
    req.format = AUDIO_F32SYS;
    req.channels = 2;
    req.samples = ap->req_bufsize;
    req.callback = sg_mixer_sdl_callback;
    dev = SDL_OpenAudioDevice(
        NULL, 0, &req, &spec,
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!dev) {
        sg_mixer_sdl_error("could not open audio device");
        return;
    }

    ap->rate = spec.freq;
    ap->bufsize = spec.samples;
    sg_logf(ap->logger, SG_LOG_INFO,
            "audio sample rate: %d", spec.freq);
    sg_logf(ap->logger, SG_LOG_INFO,
            "audio buffer size: %d", spec.samples);
    mp = sg_mixer_mixdown_new_live(spec.freq, spec.samples, &err);
    if (!mp) {
        SDL_CloseAudioDevice(dev);
        sg_logerrs(ap->logger, SG_LOG_ERROR, err,
                   "failed to create mixdown");
        sg_error_clear(&err);
        return;
    }

    ap->dev = dev;
    ap->mixdown = mp;
    SDL_PauseAudioDevice(dev, 0);
}

void
sg_mixer_stop(void)
{
    struct sg_mixer_sdl *ap = &sg_mixer_sdl;
    if (!ap->dev)
        return;
    SDL_CloseAudioDevice(ap->dev);
    ap->dev = 0;
    sg_mixer_mixdown_free(ap->mixdown);
    ap->mixdown = NULL;
}
