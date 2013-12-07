/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "defs.h"

#define ATTR(x) p->x = glGetAttribLocation(prog, #x)
#define UNIFORM(x) p->x = glGetUniformLocation(prog, #x)

void
load_prog_plain(struct prog_plain *p)
{
    GLuint prog;
    sg_opengl_checkerror("load_prog_plain start");
    p->prog = prog = load_program(
        "shader/plain.vert", "shader/plain.frag");
    ATTR(a_loc);
    UNIFORM(u_vertoff);
    UNIFORM(u_vertscale);
    UNIFORM(u_color);
    sg_opengl_checkerror("load_prog_plain");
}

void
load_prog_bkg(struct prog_bkg *p)
{
    GLuint prog;
    sg_opengl_checkerror("load_prog_bkg start");
    p->prog = prog = load_program(
        "shader/bkg.vert", "shader/bkg.frag");
    ATTR(a_loc);
    UNIFORM(u_texoff);
    UNIFORM(u_texmat);
    UNIFORM(u_tex1);
    UNIFORM(u_tex2);
    UNIFORM(u_fade);
    sg_opengl_checkerror("load_prog_bkg");
}

void
load_prog_textured(struct prog_textured *p)
{
    GLuint prog;
    sg_opengl_checkerror("load_prog_textured start");
    p->prog = prog = load_program(
        "shader/textured.vert", "shader/textured.frag");
    ATTR(a_loc);
    ATTR(a_texcoord);
    UNIFORM(u_vertoff);
    UNIFORM(u_vertscale);
    UNIFORM(u_texscale);
    UNIFORM(u_texture);
    sg_opengl_checkerror("load_prog_textured");
}

void
load_prog_text(struct prog_text *p)
{
    GLuint prog;
    sg_opengl_checkerror("load_prog_text start");
    p->prog = prog = load_program(
        "shader/text.vert", "shader/text.frag");
    ATTR(a_vert);
    UNIFORM(u_vertoff);
    UNIFORM(u_vertscale);
    UNIFORM(u_texscale);
    UNIFORM(u_texture);
    UNIFORM(u_color);
    UNIFORM(u_bgcolor);
    sg_opengl_checkerror("load_prog_text");
}

#undef ATTR
#undef UNIFORM
