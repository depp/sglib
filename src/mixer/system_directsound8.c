/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "mixer.h"
#include "sg/clock.h"
#include "sg/entry.h"
#include "sg/error.h"
#include "sg/log.h"
#include "../core/clock_impl.h"
#include <assert.h>
#include <string.h>

/* InitGuid needs to be before dsound.h */
#include <InitGuid.h>
#include <dsound.h>

#pragma comment(lib, "dsound.lib")

extern HWND sg_window;

struct sg_audio_ds8 {
    struct sg_logger *log;
    IDirectSound8 *ds;
    IDirectSoundBuffer8 *buf;
    struct sg_mixer_mixdowniface *mix;
    unsigned buflen;
    int rate;
};

static struct sg_audio_ds8 sg_audio_ds8;

#define CHECKERR() do { if (FAILED(hr)) { FAIL(herror); } } while (0)
#define FAIL(label) do { line = __LINE__; goto label; } while (0)

static void
sg_audio_ds8copy(short *dest, float *src, unsigned n)
{
    unsigned i;
    for (i = 0; i < n; ++i) {
        dest[i*2+0] = (short) (32767.0f * src[i*2+0]);
        dest[i*2+1] = (short) (32767.0f * src[i*2+1]);
    }
}

static DWORD WINAPI
sg_audio_ds8main(void *param)
{
    IDirectSoundBuffer8 *buf;
    IDirectSoundNotify8 *notp;
    struct sg_mixer_mixdowniface *mix;
    HRESULT hr;
    void *p1, *p2;
    DWORD len1, len2, dr, posplay, poswrite;
    unsigned buflen, framesz, i, period;
    struct sg_error *err = NULL;
    int line = 0;
    short *obuf;
    DSBPOSITIONNOTIFY notify[2];
    HANDLE aevt = NULL;

    buf = sg_audio_ds8.buf;
    buflen = sg_audio_ds8.buflen;
    framesz = 4;
    mix = sg_audio_ds8.mix;

    hr = IDirectSoundBuffer8_Lock(
        buf, 0, buflen * framesz * 2,
        &p1, &len1, &p2, &len2,
        DSBLOCK_ENTIREBUFFER);
    CHECKERR();
    assert(len1 == buflen * framesz * 2);

    sg_mixer_mixdown_process(mix, sg_clock_get());
    obuf = p1;
    sg_mixer_mixdown_get_s16(mix, obuf);

    sg_mixer_mixdown_process(mix, sg_clock_get());
    obuf += buflen * 2;
    sg_mixer_mixdown_get_s16(mix, obuf);

    hr = IDirectSoundBuffer8_Unlock(
        buf, p1, len1, p2, len2);
    CHECKERR();

    hr = IUnknown_QueryInterface(buf, &IID_IDirectSoundNotify8, &notp);
    CHECKERR();

    aevt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!aevt)
        FAIL(werror);
    for (i = 0; i < 2; ++i) {
        notify[i].dwOffset = buflen * framesz * i;
        notify[i].hEventNotify = aevt;
    }

    hr = IDirectSoundNotify_SetNotificationPositions(notp, 2, notify);
    CHECKERR();

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    hr = IDirectSoundBuffer8_Play(buf, 0, 0, DSBPLAY_LOOPING);
    CHECKERR();

    while (1) {
        dr = WaitForSingleObject(aevt, 1000);
        switch (dr) {
        case WAIT_OBJECT_0: break;
        case WAIT_TIMEOUT: goto timeout;
        case WAIT_FAILED: FAIL(werror);
        default: assert(0);
        }

        hr = IDirectSoundBuffer_GetCurrentPosition(
            buf, &posplay, &poswrite);
        CHECKERR();

        period = poswrite < (buflen * framesz);

        hr = IDirectSoundBuffer8_Lock(
            buf, period * buflen * framesz, buflen * framesz,
            &p1, &len1, &p2, &len2, 0);
        CHECKERR();
        assert(len1 == buflen * framesz);
        assert(len2 == 0);

        sg_mixer_mixdown_process(mix, sg_clock_get());
        obuf = p1;
        sg_mixer_mixdown_get_s16(mix, obuf);

        /* Because the event can be signaled between Wait and Lock */
        ResetEvent(aevt);

        hr = IDirectSoundBuffer8_Unlock(
            buf, p1, len1, p2, len2);
        CHECKERR();
    }

    return 0;

timeout:
    sg_logs(
        sg_audio_ds8.log, SG_LOG_WARN,
        "audio system timeout");
    goto cleanup;

werror:
    sg_error_win32(&err, GetLastError());
    goto error;

herror:
    sg_error_hresult(&err, hr);
    goto error;

error:
    sg_logerrf(
        sg_audio_ds8.log, SG_LOG_ERROR, err,
        "rendering audio (%s:%d)", __FUNCTION__, line);
    sg_error_clear(&err);
    goto cleanup;

cleanup:
    if (aevt)
        CloseHandle(notify[i].hEventNotify);
    return 0;
}

void
sg_mixer_system_init(void)
{
    sg_audio_ds8.log = sg_logger_get("audio");
}

void
sg_mixer_start(void)
{
    HRESULT hr;
    IDirectSound8 *ds = NULL;
    IDirectSoundBuffer *dbuf = NULL;
    IDirectSoundBuffer8 *dbuf8 = NULL;
    struct sg_error *err = NULL;
    struct sg_mixer_mixdowniface *mix = NULL;
    int line = 0;
    WAVEFORMATEX wfmt;
    DSBUFFERDESC dfmt;
    int rate;
    unsigned buflen, framesz;
    HANDLE thread;
    DWORD werr;

    hr = DirectSoundCreate8(NULL, &ds, NULL);
    CHECKERR();

    hr = IDirectSound8_SetCooperativeLevel(ds, sg_window, DSSCL_PRIORITY);
    CHECKERR();

    buflen = 1024;
    framesz = 4;
    rate = 48000;

    wfmt.wFormatTag = WAVE_FORMAT_PCM;
    wfmt.nChannels = 2;
    wfmt.nSamplesPerSec = rate;
    wfmt.nAvgBytesPerSec = rate * framesz;
    wfmt.nBlockAlign = framesz;
    wfmt.wBitsPerSample = 16;
    wfmt.cbSize = 0;

    dfmt.dwSize = sizeof(dfmt);
    dfmt.dwFlags =
        DSBCAPS_CTRLPOSITIONNOTIFY |
        DSBCAPS_GETCURRENTPOSITION2 |
        DSBCAPS_STICKYFOCUS;
    dfmt.dwBufferBytes = framesz * buflen * 2;
    dfmt.dwReserved = 0;
    dfmt.lpwfxFormat = &wfmt;
    dfmt.guid3DAlgorithm = GUID_NULL;

    hr = IDirectSound8_CreateSoundBuffer(ds, &dfmt, &dbuf, NULL);
    CHECKERR();

    hr = IUnknown_QueryInterface(dbuf, &IID_IDirectSoundBuffer8, &dbuf8);
    CHECKERR();

    mix = sg_mixer_mixdown_new_live(buflen, rate, &err);
    if (!mix) FAIL(error);

    sg_audio_ds8.ds = ds;
    sg_audio_ds8.buf = dbuf8;
    sg_audio_ds8.mix = mix;
    sg_audio_ds8.buflen = buflen;
    sg_audio_ds8.rate = rate;

    thread = CreateThread(NULL, 0, sg_audio_ds8main, NULL, 0, NULL);
    if (!thread) FAIL(werror);
    CloseHandle(thread);
    IUnknown_Release(dbuf);

    return;

herror:
    sg_error_hresult(&err, hr);
    goto error;

werror:
    werr = GetLastError();
    sg_error_win32(&err, werr);
    goto error;

error:
    sg_logerrf(
        sg_audio_ds8.log, SG_LOG_ERROR, err,
        "initializing audio (%s:%d)", __FUNCTION__, line);
    sg_error_clear(&err);

    if (dbuf8) IUnknown_Release(dbuf8);
    if (dbuf) IUnknown_Release(dbuf);
    if (ds) IUnknown_Release(ds);
    if (mix) sg_mixer_mixdown_free(mix);
    memset(&sg_audio_ds8, 0, sizeof(sg_audio_ds8));
    return;
}

void
sg_mixer_stop(void)
{
    sg_sys_abort("FIXME: Not implemented.");
}
