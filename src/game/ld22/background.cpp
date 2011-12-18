#include "background.hpp"
#include "defs.hpp"
#include "client/texturefile.hpp"
#include "sys/rand.hpp"
#include <memory>
#include <stdio.h>
namespace LD22 {
namespace Bkgr {

struct CloudDef {
    unsigned char x, y, w, h;

    // Call inside a GL_QUADS with the cloud texture
    void render(float px, float py) const
    {
        float x0 = px, x1 = px + 128 * w;
        float y0 = py, y1 = py + 128 * h;
        float u0 = x * 0.125f, u1 = (x + w) * 0.125f;
        float v0 = (y + h) * 0.25f, v1 = y * 0.25f;
        glTexCoord2f(u0, v0); glVertex2f(x0, y0);
        glTexCoord2f(u0, v1); glVertex2f(x0, y1);
        glTexCoord2f(u1, v1); glVertex2f(x1, y1);
        glTexCoord2f(u1, v0); glVertex2f(x1, y0);
    }
};

static const CloudDef CLOUDS[10] = {
    // big clouds
    { 0, 0, 2, 2 },
    { 2, 0, 3, 2 },
    { 5, 0, 3, 2 },
    { 3, 2, 3, 2 },

    // medium clouds
    { 0, 2, 3, 1 },
    { 0, 3, 3, 1 },

    // small clouds
    { 6, 2, 1, 1 },
    { 6, 3, 1, 1 },
    { 7, 2, 1, 1 },
    { 7, 3, 1, 1 }
};

struct CloudInstance {
    short cloud;
    short dx;
    short dy;
};

static const CloudInstance LAYER1[10] = {
    { 6,   43, 300 },
    { 7,  110, 250 },
    { 8,  177, 275 },
    { 9,  233, 290 },
    { 7,  523, 250 },
    { 6,  570, 300 },
    { 9,  817, 250 },
    { 8,  885, 300 },
    { 6,  893, 300 },
    { 7, 1004, 290 }
};

static const CloudInstance LAYER2[10] = {
    { 6,    0, 200 },
    { 6,  224, 200 },
    { 6,  283, 200 },
    { 6,  385, 200 },
    { 6,  495, 200 },
    { 6,  533, 200 },
    { 6,  640, 200 },
    { 6,  796, 200 },
    { 6,  903, 200 },
    { 6,  915, 200 }
};

static const CloudInstance LAYER3[10] = {
    { 0,   94, 50 },
    { 1,  235, 50 },
    { 2,  350, 50 },
    { 3,  508, 50 },
    { 0,  427, 50 },
    { 2,  477, 50 },
    { 3,  481, 50 },
    { 1,  535, 50 },
    { 2,  733, 50 },
    { 3,  863, 50 }
};

struct CloudLayer {
    const CloudInstance *data;
    unsigned count;
    unsigned offset;
    unsigned speed;
};

class Clouds {
    static const int LAYERS = 3;
    CloudLayer m_layer[LAYERS];
    Texture::Ref m_tex;

public:
    Clouds()
    { }

    void init()
    {
        m_tex = TextureFile::open("back/clouds.jpg");
        m_layer[0].data = LAYER1;
        m_layer[1].data = LAYER2;
        m_layer[2].data = LAYER3;
        m_layer[0].count = 10;
        m_layer[1].count = 10;
        m_layer[2].count = 10;
        for (int i = 0; i < LAYERS; ++i)
            m_layer[i].offset = Rand::girand();
        m_layer[0].speed = 3;
        m_layer[1].speed = 6;
        m_layer[2].speed = 8;
    }

    void advance()
    {
        for (int i = 0; i < LAYERS; ++i)
            m_layer[i].offset += m_layer[i].speed;
    }

    void draw(int delta)
    {
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        m_tex->bind();
        // glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
        int z = 160;
        glColor3ub(z, z, z);
        glBegin(GL_QUADS);
        for (int i = 0; i < LAYERS; ++i) {
            unsigned lpos = m_layer[i].offset +
                m_layer[i].speed * delta / FRAME_TIME;
            for (unsigned j = 0; j < m_layer[i].count; ++j) {
                const CloudInstance &ci = m_layer[i].data[j];
                int x = (-SCREEN_WIDTH / 2) + ((ci.dx * 2 + lpos) & 2047);
                int y = ci.dy;
                CLOUDS[ci.cloud].render(x, y);
            }
        }
        glEnd();
        // glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
        glColor3ub(255, 255, 255);
    }
};

class Empty : public Background {
public:
    Empty()
        : Background(EMPTY)
    { }

    virtual ~Empty()
    { }

    virtual void init()
    { }

    virtual void advance()
    { }

    virtual void draw(int delta)
    {
        float x0 = 0.0f, x1 = SCREEN_WIDTH;
        float y0 = 0.0f, y1 = SCREEN_HEIGHT;
        glColor3ub(64, 64, 64);
        glBegin(GL_QUADS);
        glVertex2f(x0, y0);
        glVertex2f(x0, y1);
        glVertex2f(x1, y1);
        glVertex2f(x1, y0);
        glEnd();
        glColor3ub(255, 255, 255);
        (void) delta;
    }
};

class Mountains : public Background {
    Texture::Ref m_tex;
    Clouds m_clouds;

public:
    Mountains()
        : Background(MOUNTAINS)
    { }

    virtual ~Mountains()
    { }

    virtual void init()
    {
        m_tex = TextureFile::open("back/mountains.jpg");
        m_clouds.init();
    }

    virtual void advance()
    {
        m_clouds.advance();
    }

    virtual void draw(int delta)
    {
        float x0 = 0.0f, x1 = SCREEN_WIDTH;
        float y0 = 0.0f, y1 = SCREEN_HEIGHT;
        float u0 = 0.0f, u1 = 1.0f;
        float v0 = 1.0f, v1 = 0.0f;
        glEnable(GL_TEXTURE_2D);
        m_tex->bind();
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex2f(x0, y0);
        glTexCoord2f(u0, v1); glVertex2f(x0, y1);
        glTexCoord2f(u1, v1); glVertex2f(x1, y1);
        glTexCoord2f(u1, v0); glVertex2f(x1, y0);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        m_clouds.draw(delta);
    }
};

}

const char *Background::getBackgroundName(int n)
{
    switch (n) {
    case EMPTY:
    default:
        return "Empty";

    case MOUNTAINS:
        return "Mountains";
    }
}

Background::~Background()
{ }

Background *Background::getBackground(int which)
{
    std::auto_ptr<Background> b;
    switch (which) {
    case EMPTY:
    default:
        b.reset(new Bkgr::Empty);
        break;

    case MOUNTAINS:
        b.reset(new Bkgr::Mountains);
        break;
    }
    b->init();
    return b.release();
}

}
