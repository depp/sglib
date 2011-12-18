#include "editor.hpp"
#include "defs.hpp"
#include "tileset.hpp"
#include "client/keyboard/keycode.h"
#include "client/opengl.hpp"
#include "client/ui/event.hpp"
#include "sys/path.hpp"
#include "client/bitmapfont.hpp"
using namespace LD22;

static const int EDITBAR_SIZE = 64;

static bool inScreen(int x, int y)
{
    return x < SCREEN_WIDTH;
}

Editor::Editor()
    : m_mode(MBrush), m_tile(0), m_mx(0), m_my(0), m_mouse(-1)
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
        setMode(MPlayer);
        break;

    default:
        break;
    }
}

void Editor::handleMouseDown(const UI::MouseEvent &evt)
{
    int x, y;
    bool inbounds;
    inbounds = translateMouse(evt, &x, &y);
    if (!inbounds)
        return;
    if (inScreen(x, y)) {
        int tx = x / TILE_SIZE, ty = y / TILE_SIZE;
        m_mx = tx;
        m_my = ty;
        m_mouse = evt.button;
        tileBrush(tx, ty);
    } else {
        
    }
}

void Editor::handleMouseUp(const UI::MouseEvent &evt)
{
    m_mouse = -1;
}

void Editor::handleMouseMove(const UI::MouseEvent &evt)
{
    if (m_mouse < 0)
        return;
    int x, y;
    bool inbounds;
    inbounds = translateMouse(evt, &x, &y);
    if (inScreen(x, y)) {
        int tx = x / TILE_SIZE, ty = y / TILE_SIZE;
        if (tx != m_mx || ty != m_my) {
            m_mx = tx;
            m_my = ty;
            tileBrush(tx, ty);
        }
    } else {
        m_mx = -1;
        m_my = -1;
    }
}

static void drawEntity(BitmapFont &f, int x, int y, const char *name)
{
    glBegin(GL_LINES);
    glVertex2s(x + 10, y);
    glVertex2s(x - 10, y);
    glVertex2s(x, y + 10);
    glVertex2s(x, y - 10);
    glEnd();
    f.print(x + 5, y + 5, name);
}

void Editor::drawExtra(int delta)
{
    BitmapFont &f = font();
    Level &l = level();
    tileset().drawTiles(level().tiles, delta);

    drawEntity(f, l.playerx, l.playery, "player");

    glPushMatrix();
    glTranslatef(SCREEN_WIDTH, 0, 0);
    int w = EDITBAR_SIZE, h = SCREEN_WIDTH;
    glBegin(GL_QUADS);
    glVertex2s(0, 0);
    glVertex2s(w, 0);
    glVertex2s(w, h);
    glVertex2s(0, h);
    glEnd();

    const char *mname = NULL;
    switch (m_mode) {
    case MBrush: mname = "brush"; break;
    case MPlayer: mname = "player"; break;
    }
    if (mname)
        f.print(5, 5, mname);

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
}

void Editor::tileBrush(int x, int y)
{
    if (x < 0 || y < 0 || x >= TILE_WIDTH || y >= TILE_HEIGHT) {
        fputs("bad tileBrush\n", stderr);
        return;
    }
    Level &l = level();
    if (m_mouse == UI::ButtonLeft) {
        l.tiles[y][x] = 1;
    } else if (m_mouse == UI::ButtonRight) {
        l.tiles[y][x] = 0;
    }
}

void Editor::open(int num)
{
    level().clear();
    try {
        level().load(num);
    } catch (Path::file_not_found &) { }
    loadLevel();
}

void Editor::save()
{
    level().save(1);
}
