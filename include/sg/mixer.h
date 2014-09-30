/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_MIXER_H
#define SG_MIXER_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
/**
 * @file sg/mixer.h
 *
 * @brief Audio mixer.
 *
 * The audio mixer provides audio playback and recording services.
 */

struct sg_mixer_sound;
struct sg_mixer_channel;

/**
 * @brief Flags for audio playback, passed to sg_mixer_channel_play().
 */
enum {
    /**
     * @brief Loop the sound until stopped.
     */
    SG_MIXER_FLAG_LOOP = 1u << 0,

    /**
     * @brief Detach the channel, freeing it automatically.
     *
     * A sound played on a detached channel will continue playing
     * until the sound ends.  The channel reference becomes invalid as
     * soon as the audio state is committed.
     */
    SG_MIXER_FLAG_DETACHED = 1u << 1
};

/**
 * @brief Mixer channel parameters.
 */
typedef enum {
    /**
     * @brief Channel volume, measured in dB.
     *
     * The values -60 and above are dB, but -80 and below are silence.
     */
    SG_MIXER_PARAM_VOL,

    /**
     * @brief Channel pan, from -1 (full left) to +1 (full right).
     */
    SG_MIXER_PARAM_PAN
} sg_mixer_param_t;

/**
 * @brief The number of mixer channel parameters.
 */
#define SG_MIXER_PARAM_COUNT ((int) SG_MIXER_PARAM_PAN + 1)

/**
 * @brief The value of a mixer channel parameter.
 */
struct sg_mixer_param {
    /**
     * @brief The parameter.
     */
    sg_mixer_param_t param;

    /**
     * @brief The parameter's value.
     */
    float value;
};

/**
 * @brief Start audio playback.
 */
void
sg_mixer_start(void);

/**
 * @brief Stop audio playback.
 */
void
sg_mixer_stop(void);

/**
 * @brief Set current mixer time.
 *
 * @param timestamp The new current mixer time.
 */
void
sg_mixer_settime(unsigned timestamp);

/**
 * @brief Commit mixer changes.
 *
 * This function must be called periodically.
 */
void
sg_mixer_commit(void);

/**
 * @brief Start recording mixer output.
 *
 * The mixer can only record to one file at a time.  The live audio
 * will not exactly match the recorded audio, due to latency and clock
 * drift in the audio system.  The recorded audio will use an exact
 * clock with no latency or drift.
 *
 * @param path Path to the output WAV file.
 * @param pathlen Length of the path, in bytes.
 * @param timestamp The timestamp at which recording starts.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_mixer_startrecord(const char *path, size_t pathlen,
                     unsigned timestamp,
                     struct sg_error **err);

/**
 * @brief Stop recording mixer output.
 *
 * This will always stop the recording process, but this may return an
 * error if the recording could not be finalized.
 *
 * @param timestamp The timestamp at which recording stops.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_mixer_stoprecord(unsigned timestamp,
                    struct sg_error **err);

/**
 * @brief Get a sound resource for an audio file.
 *
 * This will decode the the entire audio file to PCM before the sound
 * is ready for playback.  This may result in a long delay if the
 * audio file is large, e.g., if it is a music track.
 *
 * @param path Path to the audio file.
 * @param pathlen Length of the path, in bytes.
 * @param err On failure, the error.
 * @return A sound, or `NULL` for failure.  The result may be shared.
 */
struct sg_mixer_sound *
sg_mixer_sound_file(const char *path, size_t pathlen,
                    struct sg_error **err);

/**
 * @brief Test whether the sound is ready for playback.
 *
 * @param sound The sound, or `NULL` which is always ready.
 * @return Nonzero if the sound is ready, zero if the sound is not
 * ready.
 */
int
sg_mixer_sound_isready(struct sg_mixer_sound *sound);

/**
 * @brief Increment the reference count for a sound.
 *
 * @param sound The sound, or `NULL` which has no effect.
 */
void
sg_mixer_sound_incref(struct sg_mixer_sound *sound);

/**
 * @brief Decrement a reference count for a sound.
 *
 * @param sound The sound, or `NULL` which has no effect.
 */
void
sg_mixer_sound_decref(struct sg_mixer_sound *sound);

/**
 * @brief Play a sound.
 *
 * @param sound The sound to play, or `NULL` which has no effect.
 * @param timestamp The timestamp at which playback starts.
 * @param flags The playback flags, such as ::SG_MIXER_FLAG_LOOP and
 * ::SG_MIXER_FLAG_DETACHED
 * @return The sound's playback channel, or `NULL` if no channels are
 * available.  A `NULL` channel can be safely passed to the other
 * channel functions, it will have no effect.
 */
struct sg_mixer_channel *
sg_mixer_channel_play(struct sg_mixer_sound *sound,
                      unsigned timestamp, unsigned flags);

/**
 * @brief Stop channel playback, invalidating the channel.
 *
 * @param channel The mixer channel or `NULL` (which does nothing).
 */
void
sg_mixer_channel_stop(struct sg_mixer_channel *channel);

/**
 * @brief Test whether a channel is done playing.
 *
 * @param The mixer channel or `NULL` which returns nonzero.
 * @return Nonzero if playback is still in progress, zero if playback
 * has finished.
 */
int
sg_mixer_channel_isdone(struct sg_mixer_channel *channel);

/**
 * @brief Set the value of a channel parameter.
 *
 * The parameter value applies to the next commit time.  Parameters
 * are smoothly interpolated between points.
 *
 * @param channel The mixer channel or `NULL` which does nothing.
 * @param param The parameter to change.
 * @param value The new parameter value.
 */
void
sg_mixer_channel_setparam(struct sg_mixer_channel *channel,
                          sg_mixer_param_t param, float value);

/**
 * @brief Set the values of multiple channel parameters.
 *
 * The parameter value applies to the next commit time.  Parameters
 * are smoothly interpolated between points.
 *
 * @param channel The mixer channel or `NULL` which does nothing.
 * @param param The parameters to change and their values.
 * @param count The size of the parameter change list.
 */
void
sg_mixer_channel_setparams(struct sg_mixer_channel *channel,
                           struct sg_mixer_param *param, int count);

#ifdef __cplusplus
}
#endif
#endif
