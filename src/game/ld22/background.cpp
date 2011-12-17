#include "background.hpp"
#include "client/texturefile.hpp"
#include <memory>
namespace LD22 {
namespace Bkgr {

class Empty : public Background {
public:
    Empty()
    { }

    virtual ~Empty()
    { }

    virtual void init()
    { }

    virtual void advance()
    { }

    virtual void draw(int delta)
    {
        float x0 = 0.0f, x1 = 24.0f;
        float y0 = 0.0f, y1 = 15.0f;
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

public:
    Mountains()
    { }

    virtual ~Mountains()
    { }

    virtual void init()
    {
        m_tex = TextureFile::open("back/mountains.jpg");
    }

    virtual void advance()
    { }

    virtual void draw(int delta)
    {
        float x0 = 0.0f, x1 = 24.0f;
        float y0 = 0.0f, y1 = 15.0f;
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
        (void) delta;
    }
};

}

Background::~Background()
{ }

Background *Background::getBackground(Type t)
{
    std::auto_ptr<Background> b;

    switch (t) {
    case MOUNTAINS:
        b.reset(new Bkgr::Mountains);
        break;

    case EMPTY:
    default:
        b.reset(new Bkgr::Empty);
        break;
    }

    b->init();
    return b.release();
}

}
