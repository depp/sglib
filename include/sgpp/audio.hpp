/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_AUDIO_HPP
#define SGPP_AUDIO_HPP
#include "sg/audio_sample.h"
#include "sg/audio_source.h"
#include "sgpp/sharedref.hpp"

class AudioFile {
    template<class T> friend class SharedRef;
    friend class AudioSource;

protected:
    sg_audio_sample *m_ptr;

    AudioFile() : m_ptr(0) { }
    AudioFile(sg_audio_sample *ptr) : m_ptr(ptr) { }

    void incref() { if (m_ptr) sg_audio_sample_incref(m_ptr); }
    void decref() { if (m_ptr) sg_audio_sample_decref(m_ptr); }
    operator bool() const { return m_ptr != 0; }

public:
    typedef SharedRef<AudioFile> Ref;

    static Ref file(const char *path);
};

class AudioSource {
private:
    AudioSource(const AudioSource&);
    AudioSource &operator=(const AudioSource&);

    int m_id;

public:
    enum {
        NODROP = SG_AUDIO_NODROP,
        NOLATENCY = SG_AUDIO_NOLATENCY,
        LOOP = SG_AUDIO_LOOP
    };

    static const sg_audio_param_t VOL = SG_AUDIO_VOL;
    static const sg_audio_param_t PAN = SG_AUDIO_PAN;

    AudioSource()
        : m_id(-1)
    { }

    ~AudioSource()
    {
        if (m_id >= 0)
            sg_audio_source_close(m_id);
    }

    operator bool() const
    {
        return m_id >= 0;
    }

    bool open()
    {
        if (m_id >= 0)
            sg_audio_source_close(m_id);
        m_id = -1;
        m_id = sg_audio_source_open();
        return m_id >= 0;
    }

    void close()
    {
        if (m_id >= 0)
            sg_audio_source_close(m_id);
        m_id = -1;
    }

    void play(unsigned time, const AudioFile &f, int flags)
    {
        sg_audio_source_play(m_id, time, f.m_ptr, flags);
    }

    void stop(unsigned time)
    {
        sg_audio_source_stop(m_id, time);
    }

    void stoploop(unsigned time)
    {
        sg_audio_source_stoploop(m_id, time);
    }

    void pset(sg_audio_param_t p, unsigned time, float value)
    {
        sg_audio_source_pset(m_id, p, time, value);
    }

    void plinear(sg_audio_param_t p,
                 unsigned time_start, unsigned time_end,
                 float value)
    {
        sg_audio_source_plinear(m_id, p, time_start, time_end, value);
    }

    void pslope(sg_audio_param_t p, unsigned time,
                float value, float slope)
    {
        sg_audio_source_plinear(m_id, p, time, value, slope);
    }
};

#endif
