/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/opengl.h"

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
    ST_MENU, ST_IMAGE, ST_TYPE, ST_AUDIO;

struct sg_audio_pcm_obj *
load_audio(const char *path);

GLuint
load_shader(const char *path, GLenum type);

GLuint
load_texture(const char *path);

GLuint
load_program(const char *vertpath, const char *fragpath);
