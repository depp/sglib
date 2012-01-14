#include "explosion.hpp"
#include "base/rand.h"
#include "client/color.hpp"
#include "world.hpp"
#include "client/opengl.hpp"
#include <stdlib.h>
#include <cmath>
namespace Tank {

struct Explosion::Type {
    float radius, vel, randvel;
    float z, gravity, time;
    float psize, trailtime;
    unsigned int count;
    Color color;
};

const Explosion::Type Explosion::kShot = {
    0.125f, // radius
    10.0f, // vel
    0.0f, // randvel

    0.5f, // z
    9.8f, // gravity
    0.15f, // time

    0.1f, // psize
    0.1f, // trailtime

    32, // count

    Color(0, 255, 0)
};

struct Explosion::Particle {
    float loc[3];
    float vel[3];
};

Explosion::Explosion(float x, float y, const Type &type)
    : Object(0, 0, x, y, 0.0f, 0.0f),
      type_(type), time_(0.0f), particle_(0)
{ }

Explosion::~Explosion()
{
    if (particle_)
        delete[] particle_;
}

static void randvec(float v[3])
{
    float x, y, z;
    do {
        x = sg_gfrand() * 2.0f - 1.0f;
        y = sg_gfrand() * 2.0f - 1.0f;
        z = sg_gfrand() * 2.0f - 1.0f;
    } while (x*x + y*y + z*z > 1.0f);
    v[0] = x; v[1] = y; v[2] = z;
}

void Explosion::init()
{
    particle_ = new Particle[type_.count];
    for (unsigned int i = 0; i < type_.count; ++i) {
        Particle &p = particle_[i];
        randvec(p.loc);
        randvec(p.vel);
        for (unsigned int j = 0; j < 3; ++j) {
            p.vel[j] = p.vel[j] * type_.randvel + p.loc[j] * type_.vel;
            p.loc[j] = p.loc[j] * type_.radius;
        }
        p.loc[2] += type_.z;
    }
    time_ = getWorld().gameTime();
}

void Explosion::draw()
{
    glPushMatrix();
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    setupMatrix();
    float t = getWorld().gameTime() - time_;
    float trail = t < type_.trailtime ? t : type_.trailtime;
    float w = type_.psize * 0.5f;
    unsigned char c1[4], c2[4];
    for (int i = 0; i < 3; ++i)
        c1[i] = c2[i] = type_.color.c[i];
    float a = t / type_.time;
    c1[3] = (unsigned char) (255.0f * (1.0f - a * a));
    c2[3] = 0;
    float cam[3];
    getWorld().getCamera(cam);
    cam[0] -= getX();
    cam[1] -= getY();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (unsigned int i = 0; i < type_.count; ++i) {
        Particle &p = particle_[i];
        float camv[3], d[3], loc2[3], s;
        for (int j = 0; j < 3; ++j)
            loc2[j] = p.loc[j] - trail * p.vel[j];
        for (int j = 0; j < 3; ++j)
            camv[j] = p.loc[j] - cam[j];
        d[0] = p.vel[1] * camv[2] - p.vel[2] * camv[1];
        d[1] = p.vel[2] * camv[0] - p.vel[0] * camv[2];
        d[2] = p.vel[0] * camv[1] - p.vel[1] * camv[0];
        s = w / std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        d[0] *= s;
        d[1] *= s;
        d[2] *= s;
        glBegin(GL_TRIANGLE_STRIP);
        glColor4ubv(c1);
        glVertex3f(p.loc[0] - d[0], p.loc[1] - d[1], p.loc[2] - d[2]);
        glVertex3f(p.loc[0] + d[0], p.loc[1] + d[1], p.loc[2] + d[2]);
        glColor4ubv(c2);
        glVertex3f(loc2[0] - d[0], loc2[1] - d[1], loc2[2] - d[2]);
        glVertex3f(loc2[0] + d[0], loc2[1] + d[1], loc2[2] + d[2]);
        glEnd();
    }

    glPopAttrib();
    glPopMatrix();
}

void Explosion::update()
{
    if (getWorld().gameTime() > time_ + type_.time) {
        remove();
        return;
    }
    for (unsigned int i = 0; i < type_.count; ++i)
        for (unsigned int j = 0; j < 3; ++j)
            particle_[i].loc[j] += particle_[i].vel[j] * World::kFrameTime;
}

}
