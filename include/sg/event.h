/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_EVENT_H
#define SG_EVENT_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sg/event.h
 *
 * @brief Event data structures and types.
 */

/**
 * @brief Event types.
 */
typedef enum {
    /** @brief Mouse down event, uses ::sg_event_mouse.  */
    SG_EVENT_MDOWN,
    /** @brief Mouse up event, uses ::sg_event_mouse.  */
    SG_EVENT_MUP,
    /** @brief Mouse move event, uses ::sg_event_mouse.  */
    SG_EVENT_MMOVE,

    /** @brief Key down event, uses ::sg_event_key.  */
    SG_EVENT_KDOWN,
    /** @brief Key up event, uses ::sg_event_key.  */
    SG_EVENT_KUP,

    /**
     * @brief Window status event, uses ::sg_event_status.
     *
     * Indicates that the main window has changed state, the new state
     * is in the event payload.
     */
    SG_EVENT_WINDOW,

    /**
     * @brief Initialize OpenGL context, has no data.
     *
     * This event is sent before the first frame, but after the OpenGL
     * context is valid.
     */
    SG_EVENT_VIDEO_INIT,

    /**
     * @brief Destroy OpenGL context, has no data.
     *
     * This indicates that the OpenGL context has been destroyed and
     * all associated resources have been freed.
     */
    SG_EVENT_VIDEO_TERM,

    /**
     * @brief Audio system initialization event, uses
     * ::sg_event_audioinit.
     *
     * This indicates that the audio system is now initialized, and
     * the game audio callback will start being called.  Information
     * about the audio system is stored in the event payload.  This
     * event is processed synchronously by the audio system.
     */
    SG_EVENT_AUDIO_INIT,

    /**
     * @brief Audio system termination event, has no data.
     *
     * This indicates that the audio system has shut down.  In order
     * for audio to play, the audio system must be reinitialized.
     * This event is processed synchronously by the audio system.
     */
    SG_EVENT_AUDIO_TERM
} sg_event_type_t;

/**
 * @brief Mouse button constants.
 */
enum {
    /** @brief The left mouse button.  */
    SG_BUTTON_LEFT,
    /** @brief The right mouse button.  */
    SG_BUTTON_RIGHT,
    /** @brief The middle mouse button.  */
    SG_BUTTON_MIDDLE,
    /** @brief The first index for other mouse buttons.  */
    SG_BUTTON_OTHER
};

/**
 * @brief Window status flags.
 */
enum {
    /** @brief Flag indicating that the window is visible.  */
    SG_WINDOW_VISIBLE = 01,
    /** @brief Flag indicating that the window is fullscreen.  */
    SG_WINDOW_FULLSCREEN = 02
};

/**
 * @brief Mouse event.
 */
struct sg_event_mouse {
    /**
     * @brief The event type.
     *
     * Always ::SG_EVENT_MDOWN, ::SG_EVENT_MUP, or ::SG_EVENT_MMOVE.
     */
    sg_event_type_t type;

    /**
     * @brief The mouse button, if applicable.
     *
     * This is always -1 for mouse move events.
     */
    int button;

    /**
     * @brief The X coordinate of the mouse event, relative to the
     * lower left corner of the viewport.
     */
    int x;

    /**
     * @brief The Y coordinate of the mouse event, relative to the
     * lower left corner of the viewport.
     */
    int y;
};

/**
 * @brief Keyboard event.
 */
struct sg_event_key {
    /**
     * @brief The event type.
     *
     * This is always ::SG_EVENT_KDOWN or ::SG_EVENT_KUP.
     */
    sg_event_type_t type;

    /**
     * @brief The virtual key code.
     *
     * This uses a system designed to use a consistent identifier for
     * the same key across different keyboard layouts and platforms.
     * The key constants are defined in <tt>"keycode/keycode.h"</tt>.
     */
    int key;
};

/**
 * @brief Window status event.
 */
struct sg_event_status {
    /**
     * @brief The event type.
     *
     * Always ::SG_EVENT_WINDOW.
     */
    sg_event_type_t type;

    /**
     * @brief The new window status flags.
     */
    unsigned status;
};

/**
 * @brief Audio initialization event.
 */
struct sg_event_audioinit {
    /**
     * @brief The event type.
     *
     * Always ::SG_EVENT_AUDIO_INIT.
     */
    sg_event_type_t type;

    /**
     * @brief The new audio system sample rate, in Hz.
     */
    int rate;

    /**
     * @brief The buffer size for the audio system, in frames.
     */
    int buffersize;
};

/**
 * @brief Event.
 */
union sg_event {
    /**
     * @brief The event type.
     */
    sg_event_type_t type;

    /**
     * @brief The mouse event data.
     */
    struct sg_event_mouse mouse;

    /**
     * @brief The keyboard event data.
     */
    struct sg_event_key key;

    /**
     * @brief The window status event data.
     */
    struct sg_event_status status;

    /**
     * @brief The audio initialization event.
     */
    struct sg_event_audioinit audioinit;
};

#ifdef __cplusplus
}
#endif
#endif
