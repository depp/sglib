#include "area.hpp"
#include "actor.hpp"
#include "screen.hpp"
#include "tileset.hpp"
#include "level.hpp"
#include "defs.hpp"
#include "other.hpp"
#include "player.hpp"
#include "item.hpp"
#include "client/opengl.hpp"
#include "client/texturefile.hpp"
#include <cstring>
#include <algorithm>
using namespace LD22;

// Time between when all items are destroyed and when the level is
// lost.  The level can still be won in the interim.
static const int ITEM_TIMER = 25;

// Time between when all others are gone and the level is wone.  The
// level can still be lost in the interim.
static const int OTHER_TIMER = 50;

Area::Area(Screen &screen)
    : m_screen(screen)
{ }

Area::~Area()
{ }

void Area::addActor(Actor *a)
{
    m_anew.push_back(a);
    a->m_area = this;
    a->m_x0 = a->m_x;
    a->m_y0 = a->m_y;
    a->init();
}

void Area::draw(int delta)
{
    Tileset &tiles = m_screen.tileset();
    tiles.drawTiles(m_tiles, delta);
    std::vector<Actor *>::iterator
        b = m_actors.begin(), i = b, e = m_actors.end();
    for (; i != e; ++i) {
        Actor &a = **i;
        if (a.isvalid())
            a.draw(delta, tiles);
    }
}

struct ActorValid {
    bool operator()(Actor *p)
    {
        return p->isvalid();
    }
};

void Area::advance()
{
    if (m_timer > 0) {
        m_timer--;
        if (!m_timer) {
            if (!m_others)
                m_screen.win();
            else
                m_screen.lose();
            m_timer = -1;
        }
    }
    ActorValid filt;
    std::vector<Actor *>::iterator
        b = m_actors.begin(), i, e = m_actors.end(), ne;
    ne = std::stable_partition(b, e, filt);
    for (i = b; i != ne; ++i) {
        Actor &a = **i;
        if (!a.isvalid())
            continue;
        a.m_x0 = a.m_x;
        a.m_y0 = a.m_y;
        a.advance();
    }
    for (; i != e; ++i) {
        (*i)->wasDestroyed();
        delete *i;
    }
    m_actors.erase(ne, e);
    m_actors.insert(m_actors.end(), m_anew.begin(), m_anew.end());
    m_anew.clear();
}

void Area::load()
{
    clear();
    m_items = 0;
    m_others = 0;
    m_players = 0;
    m_timer = 0;
    const Level &l = m_screen.level();
    std::memcpy(m_tiles, l.tiles, sizeof(m_tiles));
    std::vector<Entity>::const_iterator
        i = l.entity.begin(), e = l.entity.end();
    for (; i != e; ++i) {
        switch (i->type) {
        case Entity::Null:
            break;

        case Entity::Player:
            addActor(new Player(i->x, i->y));
            m_players++;
            break;

        case Entity::Star:
            addActor(new Item(i->x, i->y, Item::Star));
            m_items++;
            break;

        case Entity::Other:
            addActor(new Other(i->x, i->y));
            m_others++;
            break;

        case Entity::Bomb:
            addActor(new Item(i->x, i->y, Item::Bomb));
            m_items++;
            break;
        }
    }
}

void Area::clear()
{
    std::vector<Actor *>::iterator
        i = m_actors.begin(), e = m_actors.end();
    for (; i != e; ++i)
        delete *i;
    m_actors.clear();
}

bool Area::trace(int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1, dy = y2 - y1;
    int adx = abs(dx), ady = abs(dy);
    int ad = adx > ady ? adx : ady;
    int bits, n, i;
    if (ad < 1)
        return true;
    if (ad > 1024)
        return false;
    for (bits = 0; (1 << bits) < ad; ++bits) { }
    bits -= 4;
    if (bits < 0)
        bits = 1;
    n = 1 << bits;
    for (i = 0; i < n; ++i) {
        int x = x1 + dx * (i + 1) / n;
        int y = y1 + dy * (i + 1) / n;
        if (getTile(x / TILE_SIZE, y / TILE_SIZE))
            return false;
    }
    return true;
}

void Area::removeItem()
{
    if (m_timer >= 0) {
        m_items--;
        if (!m_items && (m_timer == 0 || m_timer > ITEM_TIMER))
            m_timer = ITEM_TIMER;
    }
}

void Area::removeOther()
{
    if (m_timer >= 0) {
        m_others--;
        if (!m_others && (m_timer == 0 || m_timer > OTHER_TIMER))
            m_timer = OTHER_TIMER;
    }
}

void Area::removePlayer()
{
    if (m_timer >= 0) {
        m_players--;
        if (!m_players) {
            m_screen.lose();
            m_timer = -1;
        }
    }
}
