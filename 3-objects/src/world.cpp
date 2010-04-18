#include "world.hpp"
#include "object.hpp"
#include <cmath>
#include <limits>
#include <assert.h>
#include "SDL_opengl.h"

const float kPi = 4.0f * std::atan(1.0f);
const float World::kFrameTime = World::kFrameTicks * 0.001f;

const float kGridSpacing = 2.0f;
const int kGridSize = 8;

World::World()
    : frameNum_(0), objCount_(0), player_(0),
      playerX_(0.0f), playerY_(0.0f), playerFace_(0.0f)
{ }

World::~World()
{
    for (unsigned int i = objCount_; i > 0; --i)
        delete objects_[i - 1];
}

void World::addObject(Object *obj)
{
    assert(objCount_ < kMaxObjects);
    obj->world_ = this;
    obj->index_ = objCount_;
    objects_[objCount_] = obj;
    objCount_ += 1;
    obj->init();
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
    for (unsigned int i = 0, c = objCount_; i < c; ++i)
        objects_[i]->draw();
    glDisable(GL_DEPTH_TEST);
}

void World::update()
{
    frameNum_ += 1;
    unsigned int c = objCount_;
    for (unsigned int i = 0; i < c; ++i) {
        bool update;
        Object *p = objects_[i];
        if (p->index_ < 0)
            continue;
        p->update();
        if (p->index_ < 0)
            continue;
        float d = kFrameTime * p->speed_;
        float a = p->face_ * (kPi / 180.0f);
        float x = p->x_ + d * std::cos(a);
        float y = p->y_ + d * std::sin(a);
        float r2 = p->size_ * p->size_;
        Object *q = NULL;
        unsigned int j;
        for (j = 0; j < c; ++j) {
            if (i == j)
                continue;
            q = objects_[j];
            if (q->index_ < 0)
                continue;
            float dx = q->x_ - x, dy = q->y_ - y, dr2 = dx * dx + dy * dy;
            if (dr2 < r2 + q->size_ * q->size_)
                break;
        }
        if (j < c) {
            update = p->collide(*q);
            if (p->index_ >= 0 && q->index_ >= 0) {
                q->collide(*p);
                if (p->index_ < 0 || q->index_ < 0)
                    update = false;
            }
        } else
            update = true;
        if (update) {
            p->x_ = x;
            p->y_ = y;
        }
    }
    c = objCount_;
    for (unsigned int i = c; i > 0; --i) {
        Object *p = objects_[i - 1];
        if (p->index_ >= 0)
            continue;
        objects_[i - 1] = objects_[c - 1];
        objects_[c - 1]->index_ = i - 1;
        delete p;
        c -= 1;
    }
    objCount_ = c;
}
