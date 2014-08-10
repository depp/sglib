/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_AUDIO_FILE_H
#define SG_AUDIO_FILE_H
#include "sg/audio_buffer.h"
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
struct sg_audio_writer;
struct sg_audio_stream;
struct sg_audio_buffer;
/**
 * @file sg/audio_file.h
 *
 * @brief Audio files.
 */

/**
 * @brief List of filename extensions for audio files.
 *
 * See sg_file_open() for a description of the format of this string.
 */
extern const char SG_AUDIO_FILE_EXTENSIONS[];

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
sg_audio_file_load(struct sg_audio_buffer **buf, size_t *bufcount,
                   const void *data, size_t len,
                   struct sg_error **err);

/**
 * @brief Load a WAV file into the given buffer.
 *
 * The buffer may alias the file data.  The buffer should not be
 * initialized, and it will remain uninitialized on failure.
 *
 * @param buf The audio buffer.
 * @param data The WAV data.
 * @param len The length of the WAV data.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_audio_file_loadwav(struct sg_audio_buffer *buf,
                      const void *data, size_t len,
                      struct sg_error **err);

/**
 * @brief Load an Ogg file into an array of buffers.
 *
 * The multiple buffers exist because the Ogg file can contain more
 * than one audio stream, concatenated back to bock.
 *
 * @param buf On success, a pointer to an array of audio buffers.  The
 * array must be freed with free().
 * @param bufcount On success, the number of audio buffers.
 * @param data The Ogg data.
 * @param len The length of the Ogg data.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_audio_file_loadogg(struct sg_audio_buffer **buf, size_t *bufcount,
                      const void *data, size_t len,
                      struct sg_error **err);

/**
 * @brief Create a new WAV file and start writing to it.
 *
 * The format selected is the format of the data passed to the writer.
 * It may be converted into a different format if the requested format
 * is not supported for WAV files.
 *
 * @param path Path to the WAV file to create.
 * @param format The format of data that will be written.
 * @param samplerate The sample rate, in Hz.
 * @param channelcount The number of channels.
 * @param err On failure, the error.
 * @return An audio writer, or NULL for failure.
 */
struct sg_audio_writer *
sg_audio_writer_open(const char *path,
                     sg_audio_format_t format,
                     int samplerate, int channelcount,
                     struct sg_error **err);

/**
 * @brief Close an audio writer.
 *
 * If this fails, the audio writer will still be closed, but some data
 * was not successfully written to disk.
 *
 * @param writer The audio writer.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_audio_writer_close(struct sg_audio_writer *writer,
                      struct sg_error **err);

/**
 * @brief Write data to an audio file.
 *
 * @param writer The audio writer.
 * @param data The audio data to write.
 * @param count The number of frames of data to write.
 * @param err On failure, the error.
 * @return Zero for success, nonzero for failure.
 */
int
sg_audio_writer_write(struct sg_audio_writer *writer,
                      const void *data, int count,
                      struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
