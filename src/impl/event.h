#ifndef IMPL_EVENT_H
#define IMPL_EVENT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* sg_event_mouse */
    SG_EVENT_MDOWN,
    SG_EVENT_MUP,
    SG_EVENT_MMOVE,

    /* sg_event_key */
    SG_EVENT_KDOWN,
    SG_EVENT_KUP,

    /* sg_event_resize */
    SG_EVENT_RESIZE
} sg_event_type_t;

/* Mouse buttons */
enum {
    SG_BUTTON_LEFT,
    SG_BUTTON_RIGHT,
    SG_BUTTON_MIDDLE,
    SG_BUTTON_OTHER
};

struct sg_event_mouse {
    sg_event_type_t type;
    int button; /* Always -1 for MMOVE */
    int x, y;
};

struct sg_event_key {
    sg_event_type_t type;
    int key;
};

struct sg_event_resize {
    sg_event_type_t type;
    int width, height;
};

union sg_event {
    sg_event_type_t type;
    struct sg_event_mouse mouse;
    struct sg_event_key key;
    struct sg_event_resize resize;
};

#ifdef __cplusplus
}
#endif
#endif