#include "dict.h"
#include "error.h"
#include "texture.h"
#include "opengl.h"
#include "pixbuf.h"
#include "strbuf.h"
#include <stdlib.h>
#include <string.h>

struct sg_fmtinfo {
    GLenum ifmt, fmt, type;
};

static const struct sg_fmtinfo SG_FMTINFO[SG_PIXBUF_NFORMAT] = {
    { GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE },
    { GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE },
    { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE },
    { GL_RGB8, GL_RGB, GL_UNSIGNED_INT_8_8_8_8 },
    { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE }
};

static void
sg_texture_image_free(struct sg_resource *rs)
{
    struct sg_texture_image *img;
    GLuint texnum;

    img = (struct sg_texture_image *) rs;
    texnum = img->texnum;
    if (texnum)
        glDeleteTextures(1, &texnum);
    free(img);
}

static void *
sg_texture_image_load(struct sg_resource *rs, struct sg_error **err)
{
    struct sg_texture_image *img;
    struct sg_pixbuf *pbuf;
    int r;

    img = (struct sg_texture_image *) rs;
    pbuf = malloc(sizeof(*pbuf));
    sg_pixbuf_init(pbuf);
    if (!pbuf) {
        sg_error_nomem(err);
        return NULL;
    }
    r = sg_pixbuf_loadimage(pbuf, img->path, err);
    if (r) {
        free(pbuf);
        return NULL;
    }
    return pbuf;
}

static void
sg_texture_image_load_finished(struct sg_resource *rs, void *result)
{
    const struct sg_fmtinfo *ifo;
    struct sg_texture_image *img;
    struct sg_pixbuf *pbuf;
    GLint twidth;
    GLuint texnum;
    GLenum glerr;

    if (rs->state != SG_RSRC_LOADED)
        return;

    img = (struct sg_texture_image *) rs;
    pbuf = result;

    glGenTextures(1, &texnum);
    img->texnum = texnum;
    img->iwidth = pbuf->iwidth;
    img->iheight = pbuf->iheight;
    img->twidth = pbuf->pwidth;
    img->theight = pbuf->pheight;

    ifo = &SG_FMTINFO[(int) pbuf->format];

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texnum);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 ifo->ifmt, pbuf->pwidth, pbuf->pheight, 0,
                 ifo->fmt, ifo->type, pbuf->data);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
                             GL_TEXTURE_WIDTH, &twidth);
    glPopAttrib();

    sg_pixbuf_destroy(pbuf);
    free(pbuf);

    glerr = glGetError();
    if (glerr) {
        /* Handle this error! */
    }
}

static void
sg_texture_image_get_name(struct sg_resource *rs, struct sg_strbuf *buf)
{
    struct sg_texture_image *img = (struct sg_texture_image *) rs;
    sg_strbuf_printf(buf, "image:%s", img->path);
}

static const struct sg_resource_type sg_texture_image_type = {
    sg_texture_image_free,
    sg_texture_image_load,
    sg_texture_image_load_finished,
    sg_texture_image_get_name,
    SG_DISPATCH_IO
};

static struct dict sg_texture_images;

struct sg_texture_image *
sg_texture_image_new(const char *path, struct sg_error **err)
{
    struct dict_entry *e;
    struct sg_texture_image *img;
    size_t len;
    char *pp;

    e = dict_insert(&sg_texture_images, (char *) path);
    img = e->value;
    if (!img) {
        len = strlen(path);
        img = malloc(sizeof(*img) + len + 1);
        if (!img) {
            dict_erase(&sg_texture_images, e);
            sg_error_nomem(err);
            return NULL;
        }
        pp = (char *) (img + 1);
        img->r.type = &sg_texture_image_type;
        img->r.refcount = 0;
        img->r.action = 0;
        img->r.state = SG_RSRC_INITIAL;
        img->path = pp;
        img->texnum = 0;
        img->iwidth = 0;
        img->iheight = 0;
        img->twidth = 0;
        img->twidth = 0;
        memcpy(pp, path, len + 1);
        e->key = pp;
        e->value = img;
    }
    sg_resource_incref(&img->r);

    return img;
}
