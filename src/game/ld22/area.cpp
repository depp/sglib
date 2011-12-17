#include "area.hpp"
#include "actor.hpp"
#include "client/opengl.hpp"
#include "client/texturefile.hpp"
#include <cstring>
using namespace LD22;

Area::Area()
{
    std::memset(m_tile, 0, sizeof(m_tile));
    for (int i = 0; i < TILE_WIDTH; ++i)
        m_tile[0][i] = 1;
    m_tex = TextureFile::open("sprite/tile.png");
}

Area::~Area()
{ }

void Area::addActor(Actor *a)
{
    m_actors.push_back(a);
    a->m_area = this;
    a->m_x0 = a->m_x;
    a->m_y0 = a->m_x;
    a->init();
}

void Area::draw(int delta)
{
    glPushMatrix();
    glScalef(32.0f, 32.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_tex->bind();
    glBegin(GL_QUADS);
    for (int y = 0; y < TILE_HEIGHT; ++y) {
        for (int x = 0; x < TILE_WIDTH; ++x) {
            int t = m_tile[y][x];
            if (t) {
                int u = (t - 1) & 3, v = ((t - 1) >> 2) & 3;
                float u0 = u * 0.25f, u1 = u0 + 0.25f;
                float v1 = v * 0.25f, v0 = v1 - 0.25f;
                glTexCoord2f(u0, v0); glVertex2f(x, y);
                glTexCoord2f(u0, v1); glVertex2f(x, y + 1);
                glTexCoord2f(u1, v1); glVertex2f(x + 1, y + 1);
                glTexCoord2f(u1, v0); glVertex2f(x + 1, y);
            }
        }
    }
    glEnd();
    glPopMatrix();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    std::vector<Actor *>::iterator i = m_actors.begin(), e = m_actors.end();
    for (; i != e; ++i) {
        Actor &a = **i;
        a.draw(delta);
    }
}

void Area::advance()
{
    std::vector<Actor *>::iterator i = m_actors.begin(), e = m_actors.end();
    for (; i != e; ++i) {
        Actor &a = **i;
        a.m_x0 = a.m_x;
        a.m_y0 = a.m_y;
        a.advance();
    }
}

void Area::dumpTiles(FILE *f)
{
    fwrite(m_tile, sizeof(m_tile), 1, f);
}
