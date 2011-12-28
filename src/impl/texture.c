#include "error.h"
#include "texture.h"
#include "opengl.h"
#include "pixbuf.h"
#include <stdlib.h>
#include <string.h>

struct sg_fmtinfo {
    GLenum ifmt, fmt, type;
};

static const struct sg_fmtinfo SG_FMTINFO[SG_PIXBUF_NFORMAT] = {
    { GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE },
    { GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE },
    { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE },
    { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE }
};

int
sg_texture_load(struct sg_texture *tex, struct sg_error **err)
{
    struct sg_pixbuf pbuf;
    const struct sg_fmtinfo *ifo;
    GLuint texnum = 0;
    GLenum glerr = 0;
    GLint twidth;
    int r, ret;

    texnum = (tex->flags & SG_TEX_LOADED) ? tex->texnum : 0;

    sg_pixbuf_init(&pbuf);
    r = sg_pixbuf_loadimage(&pbuf, tex->r.name, err);
    if (r)
        goto error;

    ifo = &SG_FMTINFO[(int) pbuf.format];
    if (!texnum)
        glGenTextures(1, &texnum);
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texnum);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 ifo->ifmt, pbuf.pwidth, pbuf.pheight, 0,
                 ifo->fmt, ifo->type, pbuf.data);
    glPopAttrib();
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
                             GL_TEXTURE_WIDTH, &twidth);

    glerr = glGetError();
    if (glerr) {
        sg_error_opengl(err, glerr);
        goto error;
    }
    tex->texnum = texnum;
    tex->flags |= SG_TEX_LOADED;
    tex->iwidth = pbuf.iwidth;
    tex->iheight = pbuf.iheight;
    tex->twidth = pbuf.pwidth;
    tex->theight = pbuf.pheight;
    ret = 0;
    goto done;

error:
    if (texnum)
        glDeleteTextures(1, &texnum);
    tex->texnum = 0;
    tex->flags &= ~SG_TEX_LOADED;
    tex->iwidth = pbuf.iwidth;
    tex->iheight = pbuf.iheight;
    tex->twidth = pbuf.pwidth;
    tex->theight = pbuf.pheight;
    ret = -1;
    goto done;

done:
    sg_pixbuf_destroy(&pbuf);
    return ret;
}

struct sg_texture *
sg_texture_new(const char *path, struct sg_error **err)
{
    struct sg_texture *tex;
    size_t l;
    char *npath;
    int r;
    tex = (struct sg_texture *)
        sg_resource_find(SG_RSRC_TEXTURE, path);
    if (tex) {
        sg_resource_incref(&tex->r);
        return tex;
    }
    l = strlen(path);
    tex = malloc(sizeof(*tex) + l + 1);
    if (!tex) {
        sg_error_nomem(err);
        return NULL;
    }
    npath = (char *) (tex + 1);
    tex->r.type = SG_RSRC_TEXTURE;
    tex->r.refcount = 1;
    tex->r.flags = 0;
    tex->r.name = npath;
    tex->texnum = 0;
    tex->flags = 0;
    tex->iwidth = 0;
    tex->iheight = 0;
    tex->twidth = 0;
    tex->theight = 0;
    memcpy(npath, path, l + 1);
    r = sg_resource_register(&tex->r, err);
    if (r) {
        free(tex);
        return NULL;
    }
    return tex;
}
