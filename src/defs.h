/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/opengl.h"

/* ========================================
   Menu system
   ======================================== */

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

/* ========================================
   Resource loading
   ======================================== */

struct sg_audio_pcm_obj *
load_audio(const char *path);

GLuint
load_shader(const char *path, GLenum type);

GLuint
load_texture(const char *path);

GLuint
load_program(const char *vertpath, const char *fragpath);

/* ========================================
   Shader programs
   ======================================== */

struct prog_plain {
    GLuint prog;

    GLuint a_loc;
    GLuint u_vertoff, u_vertscale, u_color;
};

void
load_prog_plain(struct prog_plain *p);

struct prog_bkg {
    GLuint prog;

    GLint a_loc;
    GLint u_texoff, u_texmat, u_tex1, u_tex2, u_fade;
};

void
load_prog_bkg(struct prog_bkg *p);

struct prog_textured {
    GLuint prog;

    GLint a_loc, a_texcoord;
    GLint u_vertoff, u_vertscale, u_texscale, u_texture;
};

void
load_prog_textured(struct prog_textured *p);
