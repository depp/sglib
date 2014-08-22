/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_AUDIO_BUFFER_H
#define SG_AUDIO_BUFFER_H
#include "sg/attribute.h"
#include "sg/byteorder.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
/**
 * @file sg/audio_buffer.h
 *
 * @brief Audio PCM buffer.
 */

/**
 * @brief Minimum sample rate for audio samples, in hertz.
 */
#define SG_AUDIO_BUFFER_MINRATE 8000

/**
 * @brief Maximum sample rate for audio samples, in hertz.
 */
#define SG_AUDIO_BUFFER_MAXRATE 96000

/**
 * @brief Audio PCM data formats.
 */
typedef enum {
    /** @brief 8-bit unsigned samples */
    SG_AUDIO_U8,
    /** @brief 16-bit signed samples, big endian */
    SG_AUDIO_S16BE,
    /** @brief 16-bit signed samples, little endian */
    SG_AUDIO_S16LE,
    /** @brief 24-bit signed samples, big endian */
    SG_AUDIO_S24BE,
    /** @brief 24-bit signed samples, little endian */
    SG_AUDIO_S24LE,
    /** @brief 32-bit floating-point samples, big endian */
    SG_AUDIO_F32BE,
    /** @brief 32-bit floating-point samples, little endian */
    SG_AUDIO_F32LE
} sg_audio_format_t;

/**
 * @brief The number of different audio formats.
 */
#define SG_AUDIO_NFMT ((int) SG_AUDIO_F32LE + 1)

/**
 * @def SG_AUDIO_S16NE
 * @brief 16-bit signed samples, native endian
 *
 * @def SG_AUDIO_S16RE
 * @brief 16-bit signed samples, reverse endian
 *
 * @def SG_AUDIO_S24NE
 * @brief 24-bit signed samples, native endian
 *
 * @def SG_AUDIO_S24RE
 * @brief 24-bit signed samples, reverse endian
 *
 * @def SG_AUDIO_F32NE
 * @brief 32-bit floating-point samples, native endian
 *
 * @def SG_AUDIO_F32RE
 * @brief 32-bit floating-point samples, reverse endian
 */

#if PCE_BYTE_ORDER == PCE_BIG_ENDIAN
# define SG_AUDIO_S16NE SG_AUDIO_S16BE
# define SG_AUDIO_S16RE SG_AUDIO_S16LE
# define SG_AUDIO_S24NE SG_AUDIO_S24BE
# define SG_AUDIO_S24RE SG_AUDIO_S24LE
# define SG_AUDIO_F32NE SG_AUDIO_F32BE
# define SG_AUDIO_F32RE SG_AUDIO_F32LE
#else
# define SG_AUDIO_S16NE SG_AUDIO_S16LE
# define SG_AUDIO_S16RE SG_AUDIO_S16BE
# define SG_AUDIO_S24NE SG_AUDIO_S24LE
# define SG_AUDIO_S24RE SG_AUDIO_S24BE
# define SG_AUDIO_F32NE SG_AUDIO_F32LE
# define SG_AUDIO_F32RE SG_AUDIO_F32BE
#endif

/**
 * @private @brief Audio format size table.
 */
extern const unsigned char SG_AUDIO_FORMAT_SIZE[SG_AUDIO_NFMT];

/**
 * @brief Get the size of a single sample in the given format.
 */
PCE_INLINE size_t
sg_audio_format_size(sg_audio_format_t fmt)
{
    return SG_AUDIO_FORMAT_SIZE[fmt];
}

/**
 * @brief Audio PCM buffer.
 *
 * This structure may be copied or moved freely.
 */
struct sg_audio_buffer {
    /**
     * @brief Memory region holding the sample data, which must be
     * freed with free().
     *
     * This may be `NULL` if the sample data is stored in a different
     * manner.
     */
    void *alloc;

    /**
     * @brief Sample data.
     *
     * The sample data always consists of frames of interleaved
     * samples.  This will be `NULL` if the buffer has been destroyed.
     */
    const void *data;

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
     * @brief Number of frames of sample data.
     */
    int nframe;
};

/**
 * @brief Initialize a PCM buffer.
 */
void
sg_audio_buffer_init(struct sg_audio_buffer *buf);

/**
 * @brief Destroy a PCM buffer and free the associated memory.
 *
 * This function is idempotent; it is safe to destroy the same buffer
 * multiple times without reinitializing it.  This will only change
 * the `alloc` and `data` fields, other fields will be unchanged.
 */
void
sg_audio_buffer_destroy(struct sg_audio_buffer *buf);

/**
 * @brief Convert an audio buffer to the given format.
 *
 * This can only convert to signed 16-bit samples in native endian.
 *
 * @param buf The audio buffer.
 * @param format The new sample format.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_audio_buffer_convert(struct sg_audio_buffer *buf,
                        sg_audio_format_t format,
                        struct sg_error **err);

/**
 * @brief Resample an audio buffer at the given sample rate.
 *
 * This can only convert 16-bit samples in native endian.
 *
 * @param buf The audio buffer.
 * @param rate The new sample rate.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_audio_buffer_resample(struct sg_audio_buffer *buf,
                         int rate,
                         struct sg_error **err);

/**
 * @brief Get the audio data from a buffer, destroying the buffer.
 *
 * The resulting memory is a region which must be freed with `free()`.
 * If the audio buffer aliases other memory, this function will
 * allocate a new region.  In either case, the buffer is destroyed.
 *
 * @param buf The audio buffer.
 * @param err On failure, the error.
 * @return The data or `NULL` for failure.
 */
void *
sg_audio_buffer_detach(struct sg_audio_buffer *buf, struct sg_error **err);

/**
 * @brief Get the playing time of a buffer, in milliseconds.
 *
 * The result will always be rounded up.
 *
 * @param buf The audio buffer.
 * @return The playing time, in milliseconds.
 */
int
sg_audio_buffer_playtime(struct sg_audio_buffer *buf);

#ifdef __cplusplus
}
#endif
#endif
