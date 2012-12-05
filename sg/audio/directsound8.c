#include "audio_mixdown.h"
#include "audio_system.h"
#include "audio_sysprivate.h"
#include "clock.h"
#include "clock_impl.h"
#include "error.h"
#include "log.h"
#include <assert.h>
#include <string.h>

/* InitGuid needs to be before dsound.h */
#include <InitGuid.h>
#include <dsound.h>

#pragma comment(lib, "dsound.lib")

extern HWND sg_window;

struct sg_audio_ds8 {
    IDirectSound8 *ds;
    IDirectSoundBuffer8 *buf;
    struct sg_audio_mixdown *mix;
    unsigned buflen;
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
    struct sg_audio_mixdown *mix;
    HRESULT hr;
    void *p1, *p2;
    DWORD len1, len2, dr, posplay, poswrite;
    unsigned buflen, framesz, i, period;
    struct sg_error *err = NULL;
    int line = 0;
    short *obuf;
    DSBPOSITIONNOTIFY notify[2];
    float *tmpbuf = NULL;
    HANDLE aevt = NULL;

    buf = sg_audio_ds8.buf;
    buflen = sg_audio_ds8.buflen;
    framesz = 4;
    mix = sg_audio_ds8.mix;

    tmpbuf = malloc(sizeof(float) * buflen * 2);
    if (!tmpbuf) {
        sg_error_nomem(&err);
        FAIL(error);
    }

    hr = IDirectSoundBuffer8_Lock(
        buf, 0, buflen * framesz * 2,
        &p1, &len1, &p2, &len2,
        DSBLOCK_ENTIREBUFFER);
    CHECKERR();
    assert(len1 == buflen * framesz * 2);

    sg_audio_mixdown_read(mix, 0, tmpbuf);
    obuf = p1;
    sg_audio_ds8copy(obuf, tmpbuf, buflen);

    sg_audio_mixdown_read(mix, 0, tmpbuf);
    obuf += buflen * 2;
    sg_audio_ds8copy(obuf, tmpbuf, buflen);

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

        sg_audio_mixdown_read(mix, 0, tmpbuf);
        obuf = p1;
        sg_audio_ds8copy(obuf, tmpbuf, buflen);

        /* Because the event can be signaled between Wait and Lock */
        ResetEvent(aevt);

        hr = IDirectSoundBuffer8_Unlock(
            buf, p1, len1, p2, len2);
        CHECKERR();
    }
    
    return 0;

timeout:
    sg_logs(
        sg_audio_system_global.log, LOG_WARN,
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
        sg_audio_system_global.log, LOG_ERROR, err,
        "rendering audio (%s:%d)", __FUNCTION__, line);
    sg_error_clear(&err);
    goto cleanup;

cleanup:
    if (aevt)
        CloseHandle(notify[i].hEventNotify);
    free(tmpbuf);
    return 0;
}

void
sg_audio_sys_pstart(void)
{
    HRESULT hr;
    IDirectSound8 *ds = NULL;
    IDirectSoundBuffer *dbuf = NULL;
    IDirectSoundBuffer8 *dbuf8 = NULL;
    struct sg_error *err = NULL;
    struct sg_audio_mixdown *mix = NULL;
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

    mix = sg_audio_mixdown_new(rate, buflen, &err);
    if (!mix) FAIL(error);

    sg_audio_ds8.ds = ds;
    sg_audio_ds8.buf = dbuf8;
    sg_audio_ds8.mix = mix;
    sg_audio_ds8.buflen = buflen;

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
        sg_audio_system_global.log, LOG_ERROR, err,
        "initializing audio (%s:%d)", __FUNCTION__, line);
    sg_error_clear(&err);

    if (dbuf8) IUnknown_Release(dbuf8);
    if (dbuf) IUnknown_Release(dbuf);
    if (ds) IUnknown_Release(ds);
    if (mix) sg_audio_mixdown_free(mix);
    memset(&sg_audio_ds8, 0, sizeof(sg_audio_ds8));
    return;
}
