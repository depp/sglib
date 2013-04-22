union sg_event;

struct st_iface {
    const char *name;
    void (*init)();
    void (*destroy)();
    void (*event)(union sg_event *evt);
    void (*draw)(int x, int y, int width, int height, unsigned msec);
};

extern const struct st_iface *st_screen;
extern const struct st_iface
    ST_MENU, ST_IMAGE, ST_TYPE, ST_AUDIO, ST_AUDIO2;
