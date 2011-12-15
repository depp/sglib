#include "world.hpp"
#include "object.hpp"
#include <cmath>
#include <limits>
#include <assert.h>
#include <stdio.h>
#include "client/opengl.hpp"
namespace Tank {

static const unsigned int LAG_THRESHOLD = 1000;
static const unsigned int FRAME_TICKS = 10;
const float World::kFrameTime = FRAME_TICKS * 0.001f;
static const float kPi = 4.0f * std::atan(1.0f);
static const float kGridSpacing = 2.0f;
static const int kGridSize = 8;

World::World()
    : initted_(false), frameNum_(0), objCount_(0), player_(0),
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
    for (unsigned int i = 0; i < sizeof(sky) / sizeof(*sky); ++i) {
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
    glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glTranslatef(px, py, 0.0f);
    glScalef(kGridSpacing, kGridSpacing, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_LINES);
    for (int i = 0; i <= kGridSize; ++i) {
        unsigned char alpha = (kGridSize + 1 - i) * 255 / (kGridSize + 1);
        glColor4ub(51, 0, 255, alpha); glVertex2s( i,          0);
        glColor4ub(51, 0, 255,     0); glVertex2s( i, -kGridSize);
        glColor4ub(51, 0, 255, alpha); glVertex2s( i,          0);
        glColor4ub(51, 0, 255,     0); glVertex2s( i,  kGridSize);
        glColor4ub(51, 0, 255, alpha); glVertex2s(-i,          0);
        glColor4ub(51, 0, 255,     0); glVertex2s(-i, -kGridSize);
        glColor4ub(51, 0, 255, alpha); glVertex2s(-i,          0);
        glColor4ub(51, 0, 255,     0); glVertex2s(-i,  kGridSize);

        glColor4ub(51, 0, 255, alpha); glVertex2s(         0,  i);
        glColor4ub(51, 0, 255,     0); glVertex2s(-kGridSize,  i);
        glColor4ub(51, 0, 255, alpha); glVertex2s(         0,  i);
        glColor4ub(51, 0, 255,     0); glVertex2s( kGridSize,  i);
        glColor4ub(51, 0, 255, alpha); glVertex2s(         0, -i);
        glColor4ub(51, 0, 255,     0); glVertex2s(-kGridSize, -i);
        glColor4ub(51, 0, 255, alpha); glVertex2s(         0, -i);
        glColor4ub(51, 0, 255,     0); glVertex2s( kGridSize, -i);
    }
    glEnd();
    glPopAttrib();
    glPopMatrix();
}

void World::draw(int w, int h)
{
    if (player_) {
        playerX_ = player_->getX();
        playerY_ = player_->getY();
        playerFace_ = player_->getFace();
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* Calculate the perspective matrix based on 35mm equivalent focal
       length.  Different aspect ratios are scaled to match area.  */
    double f, znear, zfar, ww, hh, a;
    a = (double) w / h;
    znear = 0.1;
    zfar = 100.0;
    f = 16.0;
    hh = znear / f * sqrt(24.0 * 36.0 / a);
    ww = hh * a;
    glFrustum(-0.5*ww, 0.5*ww, -0.5*hh, 0.5*hh, znear, zfar);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawSky();

    glRotatef(90.0f - playerFace_, 0.0f, 0.0f, 1.0f);
    glTranslatef(-playerX_, -playerY_, -1.0f);

    drawGround();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    for (unsigned int i = 0, c = objCount_; i < c; ++i)
        objects_[i]->draw();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

/*
    static RasterText *t = NULL;
    if (!t) {
        t = new RasterText();
        Font f;
        static char const *const family[] = {
            "Helvetica",
            "Arial",
            "DejaVu Sans",
            "Sans",
            NULL
        };
        f.setFamily(family);
        f.setSize(12.0f);
        t->setFont(f);
    }
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Frame %u", frameNum_);
        t->setText(buf);
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, Video::width, 0.0, Video::height, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3ub(128, 255, 128);
    glTranslatef(10.0f, 10.0f, 0.0f);
    t->draw();
    glPopAttrib();
*/
}

void World::update(unsigned int ticks)
{
    if (!initted_) {
        initted_ = true;
        tickref_ = ticks;
    } else {
        unsigned int delta = ticks - tickref_, frames;
        if (delta > LAG_THRESHOLD) {
            tickref_ = ticks;
            advanceFrame();
        } else if (delta >= FRAME_TICKS) {
            frames = delta / FRAME_TICKS;
            tickref_ += frames * FRAME_TICKS;
            while (frames--)
                advanceFrame();
        }
    }
}

void World::advanceFrame()
{
    frameNum_ += 1;
    unsigned int c = objCount_;
    for (unsigned int i = 0; i < c; ++i) {
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
        bool move = true;
        for (unsigned int j = 0; j < c; ++j) {
            if (i == j)
                continue;
            Object *q = objects_[j];
            if (q->index_ < 0)
                continue;
            float dx = q->x_ - x, dy = q->y_ - y, dr2 = dx * dx + dy * dy;
            if (dr2 > r2 + q->size_ * q->size_)
                continue;
            if (p->colRcv_ & q->colGen_) {
                move = p->collide(*q) && move;
                if (p->index_ < 0)
                    break;
                if (q->index_ < 0)
                    continue;
            }
            if (q->colRcv_ & p->colGen_) {
                q->collide(*p);
                if (p->index_ < 0)
                    break;
            }
        }
        if (move) {
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
        if (player_ == p)
            player_ = NULL;
        delete p;
        c -= 1;
    }
    objCount_ = c;
}

void World::getCamera(float camera[3])
{
    camera[0] = playerX_;
    camera[1] = playerY_;
    camera[2] = 1.0f;
}

}
