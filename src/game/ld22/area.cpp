#include "area.hpp"
#include "actor.hpp"
#include "screenbase.hpp"
#include "tileset.hpp"
#include "level.hpp"
#include "defs.hpp"
#include "client/opengl.hpp"
#include "client/texturefile.hpp"
#include <cstring>
using namespace LD22;

Area::Area(ScreenBase &screen)
    : m_screen(screen)
{ }

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
    m_screen.tileset().drawTiles(m_tiles, delta);
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

void Area::load()
{
    puts("LOAD");
    std::memcpy(m_tiles, m_screen.level().tiles, sizeof(m_tiles));
}
