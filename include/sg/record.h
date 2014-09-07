/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_RECORD_H
#define SG_RECORD_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
/**
 * @file sg/record.h
 *
 * @brief Video recording.
 */

struct sg_record;

/**
 * @brief Start recording video.
 *
 * @param timestamp The timestamp at which recording starts.
 * @param width The width of the video buffer, in pixels.
 * @param height The height of the video buffer, in pixels.
 * @param err On failure, the error.
 * @return A handle for the video encoder, or `NULL` for failure.
 */
struct sg_record *
sg_record_start(unsigned timestamp, int width, int height,
                struct sg_error **err);

/**
 * @brief Stop recording video.
 *
 * The video encoding handle is disposed whether or not this function
 * succeeds.  The success of this function merely indicates the
 * success of the video encoding process as a whole.
 *
 * @param rp The video encoding handle.
 * @return Zero for success, nonzero for failure.
 */
int
sg_record_stop(struct sg_record *rp);

/**
 * @brief Get the current video encoder timestamp.
 *
 * This is advanced by sg_record_next(), not by sg_record_write().
 *
 * @param rp The video encoding handle.
 * @param time On entry, the current timestamp.  On return, the
 * encoder's timestamp.
 * @return 1 if the frame has not yet arrived but the encoder
 * recommends submitting it anyway.  2 if the frame has passed.  0 if
 * the frame has not yet passed, and the encoder does not reccommend
 * computing it in advance.
 */
int
sg_record_timestamp(struct sg_record *rp, unsigned *time);

/**
 * @brief Advance the timestamp to the next frame of video.
 *
 * @param rp The video encoding handle.
 */
void
sg_record_next(struct sg_record *rp);

/**
 * @brief Submit a frame to the encoder.
 *
 * This will eventually block if the encoder cannot keep up.  This
 * does not advance the current timestamp, you must call
 * sg_record_next() to do that.  This will take ownership of the
 * buffer pointer, and free it with free(), no matter what the return
 * value is.
 *
 * @param rp The video encoding handle.
 * @param ptr The buffer, containing RGB data, to be freed with
 * free().
 * @return One if recording is continuing, zero if recording has
 * stopped and no more frames should be submitted.
 */
int
sg_record_write(struct sg_record *rp, void *ptr);

#ifdef __cplusplus
}
#endif
#endif
