#ifndef BASE_RECORD_H
#define BASE_RECORD_H

/* Take a screenshot.  */
void
sg_record_screenshot(void);

/* Start recording video.  */
void
sg_record_vidstart(void);

/* Stop recording video.  */
void
sg_record_vidstop(void);

/* Called by the system to record the current frame for video.  */
void
sg_record_vidframe(void);

/* Fix the size of recorded frames, or (0,0) to un-fix the size.  This
   will keep video frames the same size, but it also affects
   screenshots.  */
void
sg_record_fixsize(int width, int height);

/* ====================
   Private functions for the record system
   ==================== */

/* Callback for writing a screenshot, once the pixels are in
   memory.  The pixel format is always RGBA.  */
void
sg_record_writeshot(const void *ptr, int width, int height);

/* Callback for writing a frame of video, once the pixels are in
   memory.  The pixel format is always RGBA.  */
void
sg_record_writevideo(const void *ptr, int width, int height);

void
sg_record_yuv_from_rgb(void *dest, const void *src,
                       int width, int height);

#endif
