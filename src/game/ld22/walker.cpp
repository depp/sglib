#include "area.hpp"
#include "walker.hpp"
#include "tileset.hpp"
#include "item.hpp"
#include "client/texturefile.hpp"
#include "sys/rand.hpp"
#include <stdio.h>
#include <cmath>
#include <limits>
using namespace LD22;

Walker::~Walker()
{ }

void Walker::draw(int delta, Tileset &tiles)
{
    int x, y;
    getDrawPos(&x, &y, delta);
    tiles.drawStick(x, y, m_sprite);
}

void Walker::init()
{
    checkState();
}

static int clip(int x, int n)
{
    if (x > n)
        return n;
    if (x < -n)
        return -n;
    return x;
}

static int applyPush(int speed, int push, int scale, int traction)
{
    int p = clip(push, Walker::PUSH_SCALE);
    int nspeed = p * scale * Mover::SPEED_SCALE / Walker::PUSH_SCALE;
    return speed + clip(nspeed - speed, traction);
}

void Walker::advance()
{
    static const int
        // physics
        GRAVITY = SPEED_SCALE * 3/2,
        SPEED_WALK = 12,
        SPEED_JUMP = 18,
        TRACTION_GROUND = SPEED_SCALE * 2,
        TRACTION_AIR = SPEED_SCALE * 1,
        // animation
        MIN_WALK = SPEED_SCALE * 2,
        MIN_FALL = SPEED_SCALE * 6;

    int traction = 0;
    int prevyspeed = m_ys;
    WAnim newanim = AStand;
    int atime;
    checkState();

    switch (m_wstate) {
    case WStand:
        traction = TRACTION_GROUND;
        if (m_ypush > 0)
            m_ys = SPEED_JUMP * SPEED_SCALE;
        break;

    case WFall:
        traction = TRACTION_AIR;
        m_ys -= GRAVITY;
        break;
    }
    m_xs = applyPush(m_xs, m_xpush, SPEED_WALK, traction);

    Mover::advance();

    switch (m_wstate) {
    case WStand:
        if (!m_animlock) {
            if (prevyspeed < -MIN_FALL)
                newanim = m_xs < 0 ? ALandLeft : ALandRight;
            else if (m_xs < -MIN_WALK)
                newanim = AWalkLeft;
            else if (m_xs > MIN_WALK)
                newanim = AWalkRight;
            else
                newanim = AStand;
        } else {
            newanim = m_anim;
        }
        break;

    case WFall:
        m_animlock = false;
        if (m_ys == 0 && prevyspeed < -MIN_FALL)
            newanim = m_xs < 0 ? ALandLeft : ALandRight;
        else
            newanim = AFall;
        break;
    }

    if (newanim == m_anim)
        atime = --m_animtime;
    else
        atime = -1;
    unsigned x;
    switch (newanim) {
    case AStand:
        if (atime <= 0) {
            x = Rand::girand() % 6;
            m_sprite = 0 + (x >= 3 ? x - 3 + 0x80 : x);
            m_animtime = 20 + (Rand::girand() % 40);
        }
        break;

    case AFall:
        if (atime <= 0) {
            x = Rand::girand() % 16;
            m_sprite = 16 + (x >= 8 ? x - 8 + 0x80 : x);
            m_animtime = m_ys > 0 ? 10 : 2;
        }
        break;

    case ALandLeft:
    case ALandRight:
        if (atime == 0) {
            m_sprite++;
            if ((m_sprite & 0x7f) == 15)
                m_animlock = false;
            m_animtime = 2;
        } else if (atime < 0) {
            m_sprite = 10 + (newanim == ALandLeft ? 0x80 : 0);
            m_animtime = 2;
            m_animlock = true;
        }
        break;

    case AWalkLeft:
    case AWalkRight:
        if (atime == 0) {
            m_sprite++;
            if ((m_sprite & 0x7f) == 32)
                m_sprite -= 8;
            m_animtime = 2;
        } else if (atime < 0) {
            m_sprite = 24 + (newanim == AWalkLeft ? 0x80 : 0);
            m_animtime = 2;
        }
        break;

    default:
        m_sprite = 0;
        break;
    }
    m_anim = newanim;

}

void Walker::checkState()
{
    if (m_ys <= 0 && wallAt(m_x, m_y - 1)) {
        m_wstate = WStand;
        m_ys = 0;
    } else {
        m_wstate = WFall;
    }
}

void Walker::scanItems()
{
    const std::vector<Actor *> &v = area().actors();
    std::vector<Actor *>::const_iterator i = v.begin(), e = v.end();
    int x = centerx(), y = centery();
    Item *ic = NULL;
    float icd = std::numeric_limits<float>::infinity();
    for (; i != e; ++i) {
        if ((*i)->type() != AItem)
            continue;
        Item *it = static_cast<Item *> (*i);
        int ix = it->centerx(), iy = it->centery();
        float dx = ix - x, dy = iy - y, id = dx*dx + dy*dy;
        if (id < icd) {
            ic = it;
            icd = id;
        }
    }
    if (ic) {
        m_item = ic;
        m_item_distance = std::sqrt(icd);
    } else {
        m_item = NULL;
        m_item_distance = 0.0f;
    }
}

void Walker::updateItem()
{
    if (!m_item)
        return;
    float dx = centerx() - m_item->centerx();
    float dy = centery() - m_item->centery();
    m_item_distance = std::sqrt(dx*dx + dy*dy);
}


void Walker::getGrabPos(int *x, int *y)
{
    int height;
    switch (m_sprite & 0x7f) {
    case 10:
        height = 20;
        break;

    case 11:
    case 15:
        height = 18;
        break;

    case 14:
    case 12:
        height = 16;
        break;

    case 13:
        height  = 14;
        break;

    default:
        height = 24;
        break;
    }
    *x = centerx();
    *y = centery() + STICK_HEIGHT / 2 + height;
};

bool Walker::pickupItem()
{
    if (!m_item)
        return false;
    if (m_item->m_owner != this) {
        m_item->m_owner = this;
        m_item->m_state = Item::SGrabbing;
        m_item->m_frame = 0;
    }
    return true;
}
