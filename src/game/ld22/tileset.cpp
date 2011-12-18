#include "tileset.hpp"
#include "client/opengl.hpp"
#include "client/texturefile.hpp"
using namespace LD22;

Tileset::Tileset()
{
    m_tile = TextureFile::open("sprite/tile.png");
    m_stick = TextureFile::open("sprite/stick.png");
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
            float v1 = v * 0.25f, v0 = v1 - 0.25f;
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

void Tileset::drawStick(int x, int y, int frame)
{
    int u = frame & 7, v = (frame / 8) & 3;
    bool flip = frame & 0x80;
    float x0 = x + STICK_WIDTH/2 - 32, x1 = x0 + 64;
    float y0 = y + STICK_WIDTH/2 - 32, y1 = y0 + 64;
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
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    m_stick->bind();
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(x0, y0);
    glTexCoord2f(u0, v1); glVertex2f(x0, y1);
    glTexCoord2f(u1, v1); glVertex2f(x1, y1);
    glTexCoord2f(u1, v0); glVertex2f(x1, y0);
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

}
