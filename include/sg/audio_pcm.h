/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_AUDIO_PCM_H
#define SG_AUDIO_PCM_H
#include "libpce/attribute.h"
#include "libpce/byteorder.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

/**
 * @file sg/audio_pcm.h
 *
 * @brief Audio PCM buffer.
 */

/**
 * @brief Arbitrary limit on audio file size.
 *
 * @todo Make this configurable.
 */
#define SG_AUDIO_PCM_MAXFILESZ ((size_t) 1024 * 1024 * 16)

/**
 * @brief List of filename extensions for audio files.
 *
 * See sg_file_open() for a description of the format.
 */
extern const char SG_AUDIO_PCM_EXTENSIONS[];

/**
 * @brief Minimum sample rate for audio samples, in hertz.
 */
#define SG_AUDIO_PCM_MINRATE 8000

/**
 * @brief Maximum sample rate for audio samples, in hertz.
 */
#define SG_AUDIO_PCM_MAXRATE 96000

/**
 * @brief Maximum audio file length, in seconds.
 *
 * This is 30 minutes, which is actually kind of rediculous.
 */
#define SG_AUDIO_MAXLENGTH (30*60)

/**
 * @brief PCM data formats.
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
 */
struct sg_audio_pcm {
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
sg_audio_pcm_init(struct sg_audio_pcm *buf);

/**
 * @brief Destroy a PCM buffer and free the associated memory.
 *
 * This function is idempotent; it is safe to destroy the same buffer
 * multiple times without reinitializing it.  This will only change
 * the `alloc` and `data` fields, other fields will be unchanged.
 */
void
sg_audio_pcm_destroy(struct sg_audio_pcm *buf);

/**
 * @brief Get a buffer containing the PCM data and destroy the PCM
 * buffer in the process.
 *
 * This function may or may not copy the PCM data, depending on the
 * state of the buffer.  This will only change the `alloc` and `data`
 * fields, other fields will be unchanged.
 *
 * @param buf The PCM buffer.  This buffer will be reset by this
 * operation.
 *
 * @param err On failure, the error object.
 *
 * @return On success, a pointer to a region of memory containing the
 * sample data from the PCM buffer, or `NULL` if region is empty.  The
 * caller is responsible for freeing the region with free().  On
 * failure, `NULL`.
 */
void *
sg_audio_pcm_detach(struct sg_audio_pcm *buf, struct sg_error **err);

/**
 * @brief Load an audio file.
 *
 * The audio file format is automatically detected.
 *
 * This can return multiple buffers of audio with different settings
 * (sample rate, number of channels, etc.) because this can happen
 * with concatenated audio streams in Ogg files.  The resulting
 * buffers may alias the input data if it is flat PCM.
 *
 * @param buf On success, a pointer to an array of audio buffers.  The
 * array must be freed with free().
 * @param bufcount On success, the number of audio buffers.
 * @param data The data to load.
 * @param len The length of the data to load.
 * @param err On failure, the error.
 */
int
sg_audio_pcm_load(struct sg_audio_pcm **buf, size_t *bufcount,
                  const void *data, size_t len,
                  struct sg_error **err);

/**
 * @brief Load a WAV file into the given buffer.
 *
 * The buffer may alias the file data.  The buffer should not be
 * initialized, and it will remain uninitialized on failure.
 */
int
sg_audio_pcm_loadwav(struct sg_audio_pcm *buf, const void *data, size_t len,
                     struct sg_error **err);

/**
 * @brief Load an Ogg file into an array of buffers.
 */
int
sg_audio_pcm_loadogg(struct sg_audio_pcm **buf, size_t *bufcount,
                     const void *data, size_t len,
                     struct sg_error **err);

/**
 * @brief Convert an audio buffer to the given format.
 *
 * This can only convert to signed 16-bit samples in native endian.
 */
int
sg_audio_pcm_convert(struct sg_audio_pcm *buf, sg_audio_format_t format,
                     struct sg_error **err);

/**
 * @brief Resample an audio buffer at the given sample rate.
 *
 * This can only convert 16-bit samples in native endian.
 */
int
sg_audio_pcm_resample(struct sg_audio_pcm *buf, int rate,
                      struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
