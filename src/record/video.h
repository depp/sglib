/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
struct sg_videoencoder;

/**
 * @brief Create a new video encoder.
 *
 * @param timestamp The timestamp at which recording starts.
 * @param width The width of the video buffer, in pixels.
 * @param height The height of the video buffer, in pixels.
 * @param err On failure, the error.
 * @return A handle for the video encoder, or `NULL` for failure.
 */
struct sg_videoencoder *
sg_videoencoder_new(unsigned timestamp, int width, int height,
                    struct sg_error **err);

/**
 * @brief Free a video encoder.
 *
 * The video encoding handle is disposed whether or not this function
 * succeeds.  The success of this function merely indicates the
 * success of the video encoding process as a whole.
 *
 * @param rp The video encoder handle.
 * @return Zero for success, nonzero for failure.
 */
int
sg_videoencoder_free(struct sg_videoencoder *rp);

/**
 * @brief Submit a frame to the encoder.
 *
 * This will eventually block if the encoder cannot keep up.  This
 * does not advance the current timestamp, you must call
 * sg_videoencoder_next() to do that.  This will take ownership of the
 * buffer pointer, and free it with free(), no matter what the return
 * value is.
 *
 * @param rp The video encoder handle.
 * @param ptr The buffer, containing RGB data, to be freed with
 * free().
 * @return One if recording is continuing, zero if recording has
 * stopped and no more frames should be submitted.
 */
int
sg_videoencoder_write(struct sg_videoencoder *rp, void *ptr);
