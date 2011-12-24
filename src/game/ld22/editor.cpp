#include "editor.hpp"
#include "defs.hpp"
#include "tileset.hpp"
#include "client/keyboard/keycode.h"
#include "client/opengl.hpp"
#include "client/ui/event.hpp"
// #include "sys/path.hpp"
#include "client/bitmapfont.hpp"
#include "background.hpp"
#include "sys/error.hpp"
#include <stdio.h>
#include <stdlib.h>
using namespace LD22;

static const int EDITBAR_SIZE = 64;

static bool inScreen(int x, int y)
{
    return x < SCREEN_WIDTH;
    (void) y;
}

Editor::Editor()
    : m_mode(MBrush), m_tile(1), m_mx(0), m_my(0), m_mouse(-1),
      m_ent(-1), m_etype(0)
{
    setSize(SCREEN_WIDTH + EDITBAR_SIZE, SCREEN_HEIGHT);
}

Editor::~Editor()
{ }

void Editor::init()
{
    open(1);
}

void Editor::handleEvent(const UI::Event &evt)
{
    switch (evt.type) {
    case UI::MouseDown:
        handleMouseDown(evt.mouseEvent());
        break;
    case UI::MouseUp:
        handleMouseUp(evt.mouseEvent());
        break;
    case UI::MouseMove:
        handleMouseMove(evt.mouseEvent());
        break;
    case UI::KeyDown:
        handleKeyDown(evt.keyEvent());
        break;
    default:
        break;
    }
}

void Editor::handleKeyDown(const UI::KeyEvent &evt)
{
    switch (evt.key) {
    case KEY_F1:
        save();
        break;

    case KEY_1:
        setMode(MBrush);
        break;

    case KEY_2:
        setMode(MEntity);
        break;

    case KEY_3:
        setMode(MBackground);
        break;

    case KEY_F5:
        if (m_levelno > 1)
            open(m_levelno - 1);
        break;

    case KEY_F6:
        if (m_levelno < 99)
            open(m_levelno + 1);
        break;

    case KEY_PageUp:
        incType(-1, false);
        break;

    case KEY_PageDown:
        incType(1, false);
        break;

    case KEY_Home:
        incType(-1, true);
        break;

    case KEY_End:
        incType(1, true);
        break;

    case KEY_Delete:
    case KEY_DeleteForward:
        switch (m_mode) {
        case MEntity:
            deleteEntity();
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
}

void Editor::handleMouseDown(const UI::MouseEvent &evt)
{
    int x, y, tx, ty, pe;
    bool inbounds;
    inbounds = translateMouse(evt, &x, &y);
    if (!inbounds)
        return;
    if (inScreen(x, y)) {
        switch (m_mode) {
        case MBrush:
            tx = x / TILE_SIZE;
            ty = y / TILE_SIZE;
            m_mx = tx;
            m_my = ty;
            m_mouse = evt.button;
            tileBrush(tx, ty);
            break;

        case MEntity:
            switch (evt.button) {
            case UI::ButtonLeft:
                pe = m_ent;
                selectEntity(x, y, true);
                m_mouse = (pe >= 0 && pe == m_ent) ? UI::ButtonLeft : -1;
                break;

            case UI::ButtonRight:
                newEntity(x, y);
                break;

            default:
                m_ent = -1;
                break;
            }
            break;

        case MBackground:
            break;
        }
    } else {
        
    }
}

void Editor::handleMouseUp(const UI::MouseEvent &evt)
{
    m_mouse = -1;
    (void) evt;
}

void Editor::handleMouseMove(const UI::MouseEvent &evt)
{
    if (m_mouse < 0)
        return;
    int x, y, tx, ty;
    bool inbounds;
    inbounds = translateMouse(evt, &x, &y);
    (void) inbounds;
    if (inScreen(x, y)) {
        switch (m_mode) {
        case MBrush:
            tx = x / TILE_SIZE;
            ty = y / TILE_SIZE;
            if (tx != m_mx || ty != m_my) {
                m_mx = tx;
                m_my = ty;
                tileBrush(tx, ty);
            }
            break;

        case MEntity:
            if (m_ent >= 0) {
                Entity &e = level().entity[m_ent];
                e.x = x;
                e.y = y;
            }
            break;

        case MBackground:
            break;
        }
    } else {
        m_mx = -1;
        m_my = -1;
    }
}

static void drawEntity(const Entity &e, Tileset &t)
{
    switch (e.type) {
    case Entity::Null:
        break;

    case Entity::Player:
        t.drawStick(e.x, e.y, 0, false);
        break;

    case Entity::Star:
    case Entity::EndStar:
        t.drawWidget(e.x, e.y, Widget::Star, 1.0f);
        break;

    case Entity::Other:
        t.drawStick(e.x, e.y, 0, true);
        break;

    case Entity::Bomb:
        t.drawWidget(e.x, e.y, Widget::Bomb, 1.0f);
        break;

    case Entity::EndWalker:
        t.drawStick(e.x, e.y, 0, 2);
        break;

    case Entity::EndTitle:
        break;
    }
}

void Editor::drawExtra(int delta)
{
    BitmapFont &f = font();
    Level &l = level();
    tileset().drawTiles(level().tiles, delta);

    std::vector<Entity>::const_iterator
        s = l.entity.begin(), i = s, e = l.entity.end();
    for (; i != e; ++i) {
        int idx = i - s;
        bool selected = idx == m_ent;
        int x = i->x, y = i->y;
        drawEntity(*i, tileset());
        if (selected)
            glLineWidth(5.0f);
        else
            glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex2s(x + 10, y);
        glVertex2s(x - 10, y);
        glVertex2s(x, y + 10);
        glVertex2s(x, y - 10);
        glEnd();
        glLineWidth(1.0f);
        if (selected)
            glColor3ub(255, 0, 0);
        else
            glColor3ub(0, 0, 0);
        glBegin(GL_LINES);
        glVertex2s(x + 9, y);
        glVertex2s(x - 9, y);
        glVertex2s(x, y + 9);
        glVertex2s(x, y - 9);
        glEnd();
        glColor3ub(255, 255, 255);

        f.print(x + 5, y + 5, Entity::typeName(i->type));
    }

    glPushMatrix();
    glTranslatef(SCREEN_WIDTH, 0, 0);
    int w = EDITBAR_SIZE, h = SCREEN_WIDTH;
    glBegin(GL_QUADS);
    glVertex2s(0, 0);
    glVertex2s(w, 0);
    glVertex2s(w, h);
    glVertex2s(0, h);
    glEnd();

    switch (m_mode) {
    case MBrush:
        f.print(5, 5, "brush");
        break;

    case MEntity:
        f.print(5, 5, "entity");
        f.print(5, 25, Entity::typeName((Entity::Type) m_etype));
        break;

    case MBackground:
        f.print(5, 5, "background");
        break;
    }

    glPopMatrix();
}

bool Editor::translateMouse(const UI::MouseEvent &evt, int *x, int *y)
{
    int xx = evt.x, yy = evt.y;
    convert(xx, yy);
    *x = xx;
    *y = yy;
    return xx >= 0 && xx < SCREEN_WIDTH + EDITBAR_SIZE &&
        yy >= 0 && yy < SCREEN_HEIGHT;
}

void Editor::setMode(Mode m)
{
    m_mode = m;
    m_mouse = -1;
    m_ent = -1;
}

static bool incWrap(int &v, int minv, int maxv, int delta, bool max)
{
    int vo = v, vn;
    if (max) {
        vn = delta > 0 ? maxv : minv;
    } else {
        vn = vo + delta;
        if (vn > maxv)
            vn = minv;
        else if (vn < minv)
            vn = maxv;
    }
    v = vn;
    return vo != vn;
}

void Editor::incType(int delta, bool max)
{
    switch (m_mode) {
    case MBrush:
        incWrap(m_tile, 1, MAX_TILE, delta, max);
        break;

    case MEntity:
        incWrap(m_etype, 0, Entity::MAX_TYPE, delta, max);
        break;

    case MBackground:
        incWrap(level().background, 0, Background::MAX, delta, max);
        loadLevel();
        break;
    }
}

void Editor::tileBrush(int x, int y)
{
    if (x < 0 || y < 0 || x >= TILE_WIDTH || y >= TILE_HEIGHT) {
        fputs("bad tileBrush\n", stderr);
        return;
    }
    Level &l = level();
    if (m_mouse == UI::ButtonLeft) {
        l.tiles[y][x] = m_tile;
    } else if (m_mouse == UI::ButtonRight) {
        l.tiles[y][x] = 0;
    }
}

void Editor::selectEntity(int x, int y, bool click)
{
    Level &l = level();
    unsigned n, i, off;
    if (click && m_ent >= 0) {
        Entity &e = l.entity.at(m_ent);
        if (abs(e.x - x) < 10 && abs(e.y - y) < 10)
            return;
    }
    m_mx = x;
    m_my = y;
    n = l.entity.size();
    off = n + (m_ent >= 0 ? m_ent : 0) - 1;
    for (i = 0; i < n; ++i) {
        int idx = (off - i) % n;
        Entity &e = l.entity.at(idx);
        if (abs(e.x - x) < 10 && abs(e.y - y) < 10) {
            m_ent = idx;
            return;
        }
    }
    m_ent = -1;
}

void Editor::newEntity(int x, int y)
{
    Entity e;
    e.type = (Entity::Type) m_etype;
    e.x = x;
    e.y = y;
    Level &l = level();
    m_ent = l.entity.size();
    l.entity.push_back(e);
}

void Editor::deleteEntity()
{
    if (m_ent < 0)
        return;
    Level &l = level();
    l.entity.erase(l.entity.begin() + m_ent);
    m_ent = -1;
}

void Editor::open(int num)
{
    level().clear();
    try {
        level().load(num);
    } catch (error &e) {
        if (e.is_notfound())
            return;
        throw;
    }
    loadLevel();
    m_levelno = num;
}

void Editor::save()
{
    level().save(m_levelno);
}
