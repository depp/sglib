/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_AUDIO_SAMPLE_H
#define SG_AUDIO_SAMPLE_H
#include "libpce/atomic.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

/**
 * @file sg/audio_sample.h
 *
 * @brief Audio samples.
 */

/**
 * @brief Audio sample flags.
 */
enum {
    /**
     * @brief The sample frames are interleaved stereo rather than
     * mono.
     */
    SG_AUDIO_SAMPLE_STEREO = 1u << 1,

    /**
     * @brief The sample has been converted to a different sample rate
     * from the original.
     */
    SG_AUDIO_SAMPLE_RESAMPLED = 1u << 2,
};

/**
 * @brief An audio sample.
 *
 * With the exception of private fields and the `name` field, the
 * fields in an audio sample are not valid if the sample is not
 * loaded.  Check to ensure that a sample is loaded before reading any
 * fields.
 */
struct sg_audio_sample {
    /**
     * @private @brief Audio sample reference count.
     */
    pce_atomic_t refcount;

    /**
     * @private @brief Pointer to audio sample loading state for this
     * audio sample.
     *
     * This field should not be accessed at all without the proper
     * lock.  It is `NULL` if the texture is not currently being
     * loaded.
     */
    struct sg_audio_sample_load *load;

    /**
     * @brief Audio sample name.
     *
     * This is the only public field which is valid when the sample is
     * not loaded.
     */
    const char *name;

    /**
     * @private @brief Flag which indicates whether the sample is
     * loaded.
     *
     * This flag is set with release memory ordering and should be
     * read with acquire memory ordering.  The flag may be set
     * asynchronously, but will only be cleared when no audio mixdowns
     * exist.
     *
     * The remaining fields in this structure should only be read if
     * the sample is loaded.
     */
    pce_atomic_t loaded;

    /**
     * @brief Audio sample flags.
     */
    unsigned flags;

    /**
     * @brief Length of sample, in frames.
     */
    int nframe;

    /**
     * @brief Length of sample, in milliseconds.
     */
    int playtime;

    /**
     * @brief Sample rate, in hertz.
     */
    int rate;

    /**
     * @brief Sample data, signed 16-bit native endian.
     */
    short *data;
};

/**
 * @brief Initialize the audio sample subsystem.
 */
void
sg_audio_sample_init(void);

/**
 * @private @brief Free an audio sample.
 */
void
sg_audio_sample_free_(struct sg_audio_sample *sp);

/**
 * @brief Increment an audio sample's reference count.
 */
PCE_INLINE void
sg_audio_sample_incref(struct sg_audio_sample *sp)
{
    pce_atomic_inc(&sp->refcount);
}

/**
 * @brief Decrement an audio sample's reference count.
 */
PCE_INLINE void
sg_audio_sample_decref(struct sg_audio_sample *sp)
{
    int c = pce_atomic_fetch_add(&sp->refcount, -1);
    if (c == 1)
        sg_audio_sample_free_(sp);
}

/**
 * @brief Check whether the audio sample is loaded.
 */
PCE_INLINE int
sg_audio_sample_is_loaded(struct sg_audio_sample *sp)
{
    return pce_atomic_get_acquire(&sp->loaded);
}

/**
 * @brief Create an audio sample from the file at the given path.
 *
 * The sample may be loaded asynchronously.
 */
struct sg_audio_sample *
sg_audio_sample_file(const char *path, size_t pathlen,
                     struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
