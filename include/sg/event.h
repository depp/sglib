/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_EVENT_H
#define SG_EVENT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* pce_event_mouse */
    SG_EVENT_MDOWN,
    SG_EVENT_MUP,
    SG_EVENT_MMOVE,

    /* pce_event_key */
    SG_EVENT_KDOWN,
    SG_EVENT_KUP,

    /* pce_event_resize */
    SG_EVENT_RESIZE,

    /* pce_event_status */
    SG_EVENT_STATUS
} pce_event_type_t;

/* Mouse buttons */
enum {
    SG_BUTTON_LEFT,
    SG_BUTTON_RIGHT,
    SG_BUTTON_MIDDLE,
    SG_BUTTON_OTHER
};

/* Possible status flags */
enum {
    SG_STATUS_VISIBLE = 01,
    SG_STATUS_FULLSCREEN = 02
};

struct pce_event_mouse {
    pce_event_type_t type;
    int button; /* Always -1 for MMOVE */
    int x, y;
};

struct pce_event_key {
    pce_event_type_t type;
    int key;
};

struct pce_event_resize {
    pce_event_type_t type;
    int width, height;
};

struct pce_event_status {
    pce_event_type_t type;
    unsigned status;
};

union pce_event {
    pce_event_type_t type;
    struct pce_event_mouse mouse;
    struct pce_event_key key;
    struct pce_event_resize resize;
    struct pce_event_status status;
};

#ifdef __cplusplus
}
#endif
#endif
