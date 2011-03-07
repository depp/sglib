#include "dummytexture.hpp"
#include <string.h>

DummyTexture DummyTexture::nullTexture
("<null>", Color(255, 0, 255), Color(32, 32, 32));

DummyTexture::~DummyTexture()
{ }

bool DummyTexture::load()
{
    unsigned char *buf, *p, c1[3], c2[3];
    unsigned int w = 16, h = 16, rowbytes, x, y;
    memcpy(c1, c1_.c, sizeof(c1));
    memcpy(c2, c2_.c, sizeof(c2));
    alloc(w, h, true, false);
    buf = reinterpret_cast<unsigned char *>(this->buf());
    rowbytes = this->rowbytes();
    for (y = 0; y < h / 2; ++y) {
        p = buf + y * rowbytes;
        for (x = 0; x < w / 2; ++x) {
            p[x*3+0] = c1[0];
            p[x*3+1] = c1[1];
            p[x*3+2] = c1[2];
        }
        for (; x < w; ++x) {
            p[x*3+0] = c2[0];
            p[x*3+1] = c2[1];
            p[x*3+2] = c2[2];
        }
    }
    for (; y < h; ++y) {
        p = buf + y * rowbytes;
        for (x = 0; x < w / 2; ++x) {
            p[x*3+0] = c2[0];
            p[x*3+1] = c2[1];
            p[x*3+2] = c2[2];
        }
        for (; x < w; ++x) {
            p[x*3+0] = c1[0];
            p[x*3+1] = c1[1];
            p[x*3+2] = c1[2];
        }
    }
    return true;
}

std::string DummyTexture::name() const
{
    return name_;
}
