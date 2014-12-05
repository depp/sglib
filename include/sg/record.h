/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_RECORD_H
#define SG_RECORD_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;
/**
 * @file sg/record.h
 *
 * @brief Video recording and screenshots.
 */

/**
 * @brief Finish rendering a frame.
 *
 * This should be called with a valid OpenGL context.  The contents of
 * the framebuffer will be recorded, if video recording is in progress
 * or a screenshot is being taken.  This will automatically get called
 * at the end of every frame, but by calling this function explicitly,
 * you can specify the capture region.
 *
 * @param x The x offset of the capture region in the framebuffer.
 * @param y The y offset of the capture region in the framebuffer.
 * @param width The width of the capture region in the framebuffer.
 * @param height The height of the capture region in the framebuffer.
 */
void
sg_record_frame_end(int x, int y, int width, int height);

/**
 * @brief Request a screenshot of the next frame.
 */
void
sg_record_screenshot(void);

/**
 * @brief Start recording video.
 *
 * @param time Timestamp at which video recording starts.
 * @param width Width of the video, in pixels.
 * @param height Height of the video, in pixels.
 */
void
sg_record_start(double time);

/**
 * @brief Stop recording video.
 */
void
sg_record_stop(void);

#ifdef __cplusplus
}
#endif
#endif
