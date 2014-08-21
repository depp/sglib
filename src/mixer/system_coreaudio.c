/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "clock_impl.h"
#include "sg/clock.h"
#include "sg/entry.h"
#include "sg/log.h"
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <math.h>

#define SG_AUDIO_COREAUDIO_MINRATE 8000
#define SG_AUDIO_COREAUDIO_MAXRATE 200000

static struct sg_audio_mixdown *sg_audio_ca_mixdown;
static struct sg_logger *sg_audio_ca_logger;

static OSStatus
sg_audio_callback(
    void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList * ioData)
{
    unsigned time;
    AudioBuffer *buf;
    (void) inRefCon;
    (void) ioActionFlags;
    (void) inTimeStamp;
    (void) inNumberFrames;
    (void) inBusNumber;

    if (ioData->mNumberBuffers < 1)
        return 0;
    time = sg_clock_convert(inTimeStamp->mHostTime);
    buf = &ioData->mBuffers[0];
    if (buf->mNumberChannels != 2)
        return 0;
    sg_audio_mixdown_read(sg_audio_ca_mixdown, time, buf->mData);
    return 0;
}

void
sg_audio_sys_pstart(void)
{
    ComponentDescription desc;
    Component comp;
    AudioUnit output;
    AudioStreamBasicDescription sfmt;
    AURenderCallbackStruct callback;
    OSStatus e = 0;
    Float64 sampleRate;
    UInt32 size, bufsize;
    int isampleRate;
    const char *why, *ewhy;
    struct sg_error *err = NULL;

    sg_audio_ca_logger = sg_logger_get("audio");

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    comp = FindNextComponent(NULL, &desc);
    if (!comp) {
        why = "could not get output component";
        goto error;
    }

    e = OpenAComponent(comp, &output);
    if (e) {
        why = "could not open output component";
        goto error;
    }

    e = AudioUnitInitialize(output);
    if (e) {
        why = "could not initialize audio unit";
        goto error;
    }

    callback.inputProc = sg_audio_callback;
    callback.inputProcRefCon = NULL;
    e = AudioUnitSetProperty(
        output,
        kAudioUnitProperty_SetRenderCallback,
        kAudioUnitScope_Input,
        0,
        &callback, sizeof(callback));
    if (e) {
        why = "could not set callback";
        goto error;
    }

    size = sizeof(sampleRate);
    e = AudioUnitGetProperty(
        output,
        kAudioUnitProperty_SampleRate,
        kAudioUnitScope_Output,
        0,
        &sampleRate, &size);
    if (e) {
        why = "could not get property";
        goto error;
    }
    isampleRate = (int) floor(sampleRate + 0.5);
    if (isampleRate < SG_AUDIO_COREAUDIO_MINRATE ||
        isampleRate > SG_AUDIO_COREAUDIO_MAXRATE)
    {
        why = "sample rate is too extreme";
        goto error;
    }

    sfmt.mSampleRate = sampleRate;
    sfmt.mFormatID = kAudioFormatLinearPCM;
    sfmt.mFormatFlags =
        kAudioFormatFlagIsFloat |
        kAudioFormatFlagsNativeEndian |
        kAudioFormatFlagIsPacked;
    sfmt.mBytesPerPacket = 8;
    sfmt.mFramesPerPacket = 1;
    sfmt.mBytesPerFrame = 8;
    sfmt.mChannelsPerFrame = 2;
    sfmt.mBitsPerChannel = 32;
    sfmt.mReserved = 0;
    e = AudioUnitSetProperty(
        output,
        kAudioUnitProperty_StreamFormat,
        kAudioUnitScope_Input,
        0,
        &sfmt, sizeof(sfmt));
    if (e) {
        why = "could not set stream format";
        goto error;
    }

    size = sizeof(bufsize);
    e = AudioUnitGetProperty(
        output,
        kAudioDevicePropertyBufferFrameSize,
        kAudioUnitScope_Global,
        0,
        &bufsize,
        &size);
    if (e) {
        why = "could not get buffer size";
        goto error;
    }

    sg_logf(sg_audio_ca_logger, SG_LOG_INFO,
            "audio buffer size: %u", bufsize);
    sg_logf(sg_audio_ca_logger, SG_LOG_INFO,
            "audio sample rate: %g", sampleRate);

    sg_audio_ca_mixdown = sg_audio_mixdown_new(
        isampleRate, bufsize, &err);
    if (!sg_audio_ca_mixdown) {
        why = "could not create audio system";
        goto error2;
    }

    e = AudioOutputUnitStart(output);
    if (e) {
        why = "could not start audio output";
        goto error;
    }

    return;

error:
    if (!e) goto simple_error;
    ewhy = GetMacOSStatusErrorString(e);
    if (ewhy && *ewhy) {
        sg_logf(sg_audio_ca_logger, LOG_ERROR, "%s (%s)", why, ewhy);
    } else {
        char buf[5];
        int i;
        for (i = 0; i < 4; ++i) {
            buf[i] = e >> ((3 - i) * 8);
            if (buf[i] < 0x20 || buf[i] > 0x7e)
                break;
        }
        if (i == 4)
            sg_logf(sg_audio_ca_logger, LOG_ERROR, "%s (%s)", buf);
        else
            sg_logf(sg_audio_ca_logger, LOG_ERROR, "%s (%d)", why, e);
    }
    goto cleanup;

error2:
    sg_logerrs(sg_audio_ca_logger, LOG_ERROR, err, why);
    goto cleanup;

simple_error:
    sg_logs(sg_audio_ca_logger, LOG_ERROR, why);
    goto cleanup;

cleanup:
    /* FIXME: do actual cleanup here */
    return;
}
