/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_AUDIO_STREAM_H
#define SG_AUDIO_STREAM_H
#include "sg/audio_buffer.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio PCM stream.
 *
 * This structure may be copied or moved freely.
 */
struct sg_audio_stream {
    /**
     * @brief Callback parameter.
     */
    void *data;

    /**
     * @brief Free the stream.
     *
     * @param data The callback parameter.
     */
    void
    (*free)(void *data);

    /**
     * @brief Read a buffer of data from the stream.
     *
     * @param data The callback parameter.
     * @param buffer The buffer to fill.
     * @return An audio stream result code.
     */
    sg_audio_result_t
    (*read)(void *data, void *buffer);

    /**
     * @brief Sample format.
     */
    sg_audio_format_t format;

    /**
     * @brief Sample rate, in hertz.
     */
    int rate;

    /**
     * @brief Number of channels.
     */
    int nchan;

    /**
     * @brief Number of frames of sample data in each buffer.
     */
    int buffersize;

    /**
     * @brief Total stream length, in frames, or -1 for unknown stream
     * length.
     */
    int streamlength;
};

#ifdef __cplusplus
}
#endif
#endif
