/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_AUDIO_H
#define SG_AUDIO_H
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file sg/audio.h
 *
 * @brief Audio definitions.
 */

/**
 * @brief Return codes for audio stream functions.
 *
 * Audio stream functions fill a buffer with data and return one of
 * these result codes.
 */
typedef enum {
    /**
     * @brief The audio stream has stopped.
     *
     * The buffer was not filled, and no more data will be returned.
     */
    SG_AUDIO_STOP,

    /**
     * @brief The audio stream is only silence.
     *
     * The buffer was not filled, and silence should be played in its
     * place.
     */
    SG_AUDIO_SILENCE,

    /**
     * @brief The audio buffer contains silence.
     *
     * The buffer was filled, but it contains silence.
     */
    SG_AUDIO_DATA_SILENCE,

    /**
     * @brief The audio buffer contains data.
     *
     * The buffer was filled, and it may contain data other than
     * silence.
     */
    SG_AUDIO_DATA
} sg_audio_result_t;

#ifdef __cplusplus
}
#endif
#endif
