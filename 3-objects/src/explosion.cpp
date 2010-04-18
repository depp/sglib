#include "explosion.hpp"
#include "rand.hpp"
#include "color.hpp"
#include "world.hpp"
#include "SDL_opengl.h"

struct Explosion::Type {
    float r, z, v, g, t, psz;
    unsigned int n;
    Color color;
};

const Explosion::Type Explosion::kShot = {
    0.25f, 0.5f, 15.0f, 9.8f, 0.15f, 2.0f,
    128,
    Color::lime
};

Explosion::Explosion(float x, float y, const Type &type)
    : Object(0, 0, x, y, 0.0f, 0.0f),
      type_(type), time_(0.0f), vertex_(0)
{ }

Explosion::~Explosion()
{
    if (vertex_)
        delete[] vertex_;
}

void Explosion::init()
{
    short (*vp)[3], x, y, z;
    unsigned int n = type_.n, r;
    vertex_ = vp = new short[n][3];
    for (; n > 0; --n, ++vp) {
        do {
            x = Rand::girand();
            y = Rand::girand();
            z = Rand::girand();
            r = (int)(x / 2) * (x / 2) +
                (int)(y / 2) * (y / 2) +
                (int)(z / 2) * (z / 2);
        } while (r > (1U << 28));
        (*vp)[0] = x;
        (*vp)[1] = y;
        (*vp)[2] = z;
    }
    time_ = getWorld().gameTime();
}

void Explosion::draw()
{
    glPushMatrix();
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_POINT_BIT);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    setupMatrix();
    float t = getWorld().gameTime() - time_;
    glTranslatef(0.0, 0.0, type_.z - 0.5 * type_.g * t * t);
    float s = (type_.r + type_.v * t) / 32768.0f;
    glScalef(s, s, s);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(type_.color.c[0], type_.color.c[1], type_.color.c[2],
               255.0f * (1.0f - t / type_.t));
    glPointSize(type_.psz);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_SHORT, 0, vertex_);
    glDrawArrays(GL_POINTS, 0, type_.n);

    glPopClientAttrib();
    glPopAttrib();
    glPopMatrix();
}

void Explosion::update()
{
    if (getWorld().gameTime() > time_ + type_.t)
        remove();
}
