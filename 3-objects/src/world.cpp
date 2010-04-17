#include "world.hpp"
#include "object.hpp"
#include <cmath>
#include <limits>
#include "SDL_opengl.h"

#include <stdio.h>

const float kPi = 4.0f * std::atan(1.0f);
const float World::kFrameTime = World::kFrameTicks * 0.001f;

const float kGridSpacing = 2.0f;
const int kGridSize = 8;

World::World()
    : first_(0), player_(0),
      playerX_(0.0f), playerY_(0.0f), playerFace_(0.0f),
      frameNum_(0)
{ }

World::~World()
{
    Object *p = first_, *n;
    while (p) {
        n = p->next_;
        delete p;
        p = n;
    }
}

void World::addObject(Object *obj)
{
    obj->world_ = this;
    obj->next_ = first_;
    first_ = obj;
}

void World::setPlayer(Object *obj)
{
    player_ = obj;
}

struct gradient_point {
    float pos;
    unsigned char color[3];
};

const gradient_point sky[] = {
    { -0.50f, {   0,   0,  51 } },
    { -0.02f, {   0,   0,   0 } },
    {  0.00f, { 102, 204, 255 } },
    {  0.20f, {  51,   0, 255 } },
    {  0.70f, {   0,   0,   0 } }
};

void World::drawSky(void)
{
    glPushAttrib(GL_CURRENT_BIT);
    glBegin(GL_TRIANGLE_STRIP);
    glColor3ubv(sky[0].color);
    glVertex3f(-2.0f, 1.0f, -1.0f);
    glVertex3f( 2.0f, 1.0f, -1.0f);
    for (int i = 0; i < sizeof(sky) / sizeof(*sky); ++i) {
        glColor3ubv(sky[i].color);
        glVertex3f(-2.0f, 1.0f, sky[i].pos);
        glVertex3f( 2.0f, 1.0f, sky[i].pos);
    }
    glVertex3f(-2.0f, 0.0f, 1.0f);
    glVertex3f( 2.0f, 0.0f, 1.0f);
    glEnd();
    glPopAttrib();
}

void World::drawGround(void)
{
    float px = std::floor(playerX_ / kGridSpacing + 0.5f) * kGridSpacing;
    float py = std::floor(playerY_ / kGridSpacing + 0.5f) * kGridSpacing;
    glPushAttrib(GL_CURRENT_BIT);
    glPushMatrix();
    glTranslatef(px, py, 0.0f);
    glScalef(kGridSpacing, kGridSpacing, 1.0f);
    glColor3ub(51, 0, 255);
    glBegin(GL_LINES);
    for (int i = -kGridSize; i <= kGridSize; ++i) {
        glVertex3f(i, -kGridSize, 0.0f);
        glVertex3f(i,  kGridSize, 0.0f);
        glVertex3f(-kGridSize, i, 0.0f);
        glVertex3f( kGridSize, i, 0.0f);
    }
    glEnd();
    glPopAttrib();
    glPopMatrix();
}


void World::draw()
{
    if (player_) {
        playerX_ = player_->getX();
        playerY_ = player_->getY();
        playerFace_ = player_->getFace();
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.1f, 0.1f, -0.075f, 0.075f, 0.1f, 100.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawSky();

    glRotatef(90.0f - playerFace_, 0.0f, 0.0f, 1.0f);
    glTranslatef(-playerX_, -playerY_, -1.0f);

    drawGround();

    glEnable(GL_DEPTH_TEST);
    for (Object *p = first_; p; p = p->next_)
        p->draw();
    glDisable(GL_DEPTH_TEST);
}

void World::update()
{
    frameNum_ += 1;
    for (Object *p = first_; p; p = p->next_) {
        float d = kFrameTime * p->speed_;
        float a = p->face_ * (kPi / 180.0f);
        float x = p->x_ + d * std::cos(a);
        float y = p->y_ + d * std::sin(a);
        float r2 = p->size_ * p->size_;
        Object *q = first_;
        for (; q; q = q->next_) {
            if (p == q)
                continue;
            float dx = q->x_ - x, dy = q->y_ - y, dr2 = dx * dx + dy * dy;
            if (dr2 < r2 + q->size_ * q->size_)
                break;
        }
        if (q) {
            // collision
        } else {
            p->x_ = x;
            p->y_ = y;
        }
        p->update();
    }
}
