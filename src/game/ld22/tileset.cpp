#include "tileset.hpp"
#include "client/opengl.hpp"
#include "client/texturefile.hpp"
#include <stdlib.h>
using namespace LD22;

Tileset::Tileset()
{
    m_tile = TextureFile::open("sprite/tile.png");
    m_stick = TextureFile::open("sprite/stick.png");
    m_widget = TextureFile::open("sprite/widget.png");
}

Tileset::~Tileset()
{ }

void Tileset::drawTiles(const unsigned char t[TILE_HEIGHT][TILE_WIDTH],
                        int delta) const
{
    (void) delta;

    glPushMatrix();
    glScalef(32.0f, 32.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_tile->bind();
    glBegin(GL_QUADS);
    for (int y = 0; y < TILE_HEIGHT; ++y) {
        for (int x = 0; x < TILE_WIDTH; ++x) {
            int i = t[y][x];
            if (!i)
                continue;
            int u = (i - 1) & 3, v = ((i - 1) >> 2) & 3;
            float u0 = u * 0.25f, u1 = u0 + 0.25f;
            float v1 = v * 0.25f, v0 = v1 + 0.25f;
            glTexCoord2f(u0, v0); glVertex2f(x, y);
            glTexCoord2f(u0, v1); glVertex2f(x, y + 1);
            glTexCoord2f(u1, v1); glVertex2f(x + 1, y + 1);
            glTexCoord2f(u1, v0); glVertex2f(x + 1, y);
        }
    }
    glEnd();
    glPopMatrix();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

static const float STICK_COLOR[2][4] = {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.2f, 0.2f, 0.7f, 1.0f }
};

void Tileset::drawStick(int x, int y, int frame, bool isOther)
{
    const float *color;
    if (isOther)
        color = STICK_COLOR[1];
    else
        color = STICK_COLOR[0];
    int u = frame & 7, v = (frame / 8) & 3;
    bool flip = frame & 0x80;
    float x0 = x + STICK_WIDTH/2 - 32, x1 = x0 + 64;
    float y0 = y + STICK_HEIGHT/2 - 32, y1 = y0 + 64;
    float u0 = u * 0.125f, u1 = u0 + 0.125f;
    float v1 = v * 0.25f, v0 = v1 + 0.25f;
    if (flip) {
        float t;
        t = u0;
        u0 = u1;
        u1 = t;
    }
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glBlendColor(color[0], color[1], color[2], color[3]);
    glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR);
    m_stick->bind();
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(x0, y0);
    glTexCoord2f(u0, v1); glVertex2f(x0, y1);
    glTexCoord2f(u1, v1); glVertex2f(x1, y1);
    glTexCoord2f(u1, v0); glVertex2f(x1, y0);
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
}

static const int IOFF = (64 - ITEM_SIZE) / 2;

static const signed char WIDGET_INFO[Widget::MAX_WIDGET + 1][6] = {
    { 2, 2, -2, -2, 0, 0 },
    { 0, 2, 2, -2, 0, 0 },
    { 2, 2, 2, -2, 0, 0 },
    { 4, 2, -2, -2, 0, 0 },
    { 0, 3, 1, -1, -IOFF, -IOFF },
    { 1, 3, 1, -1, -IOFF, -IOFF },
    { 2, 3, 1, -1, -IOFF, -IOFF },
    { 2, 4, 1, -1, -IOFF, -IOFF },
    { 3, 4, 1, -2, -IOFF, -IOFF },
    { 1, 4, 1, -1, -IOFF, -IOFF },
    { 0, 4, 1, -1, -IOFF, -IOFF }
};

void Tileset::drawWidget(int x, int y, int which, float scale)
{
    if (which < 0 || which > Widget::MAX_WIDGET)
        return;
    const signed char *ifo = WIDGET_INFO[which];
    float x0 = x + ifo[4], x1 = x0 + 64 * abs(ifo[2]);
    float y0 = y + ifo[5], y1 = y0 + 64 * abs(ifo[3]);
    float u0 = 0.25f * ifo[0], u1 = u0 + 0.25f * ifo[2];
    float v0 = 0.25f * ifo[1], v1 = v0 + 0.25f * ifo[3];
    float f = 0.5f * (1.0f - scale);
    float dx = f * (x1 - x0), dy = f * (y1 - y0);
    x0 += dx;
    x1 -= dx;
    y0 += dy;
    y1 -= dy;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_widget->bind();
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(x0, y0);
    glTexCoord2f(u0, v1); glVertex2f(x0, y1);
    glTexCoord2f(u1, v1); glVertex2f(x1, y1);
    glTexCoord2f(u1, v0); glVertex2f(x1, y0);
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void Tileset::loadEnd()
{
    m_end = TextureFile::open("menu/fin.jpg");
}

void Tileset::drawEnd(int x, int y, int which, float scale)
{
    float u0 = 0.0f, u1 = 1.0f;
    float v0 = 0.0f, v1 = 1.0f;
    switch (which) {
    case 0: v0 = 0.50f; v1 = 0.00f; break;
    case 1: v0 = 0.75f; v1 = 0.50f; break;
    case 2: v0 = 1.00f; v1 = 0.75f; break;
    }
    float x0 = x, x1 = x + scale;
    float y0 = y, y1 = y + scale * (which == 0 ? 1.0f : 0.5f);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
    m_end->bind();
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(x0, y0);
    glTexCoord2f(u0, v1); glVertex2f(x0, y1);
    glTexCoord2f(u1, v1); glVertex2f(x1, y1);
    glTexCoord2f(u1, v0); glVertex2f(x1, y0);
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}
