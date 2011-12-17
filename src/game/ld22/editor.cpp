#include "editor.hpp"
#include "defs.hpp"
#include "area.hpp"
#include "client/opengl.hpp"
#include "client/ui/event.hpp"
#include <stdio.h>
using namespace LD22;

static const int EDITBAR_SIZE = 64;

static bool inScreen(int x, int y)
{
    return x < SCREEN_WIDTH;
}

Editor::Editor()
    : m_tile(0), m_mx(0), m_my(0), m_mouse(-1)
{
    setSize(SCREEN_WIDTH + EDITBAR_SIZE, SCREEN_HEIGHT);
}

Editor::~Editor()
{ }

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
    (void) evt;
}

void Editor::handleMouseDown(const UI::MouseEvent &evt)
{
    int x, y;
    bool inbounds;
    inbounds = translateMouse(evt, &x, &y);
    if (!inbounds)
        return;
    if (inScreen(x, y)) {
        Area &a = area();
        int tx = x / TILE_SIZE, ty = y / TILE_SIZE;
        m_mx = tx;
        m_my = ty;
        a.setTile(tx, ty, 1);
    } else {
        
    }
}

void Editor::handleMouseUp(const UI::MouseEvent &evt)
{
    int x, y;
    bool inbounds;
    inbounds = translateMouse(evt, &x, &y);
}

void Editor::handleMouseMove(const UI::MouseEvent &evt)
{
    if (m_mouse < 0)
        return;
    int x, y;
    bool inbounds;
    inbounds = translateMouse(evt, &x, &y);
}

void Editor::drawExtra()
{
    glTranslatef(SCREEN_WIDTH, 0, 0);
    int w = EDITBAR_SIZE, h = SCREEN_WIDTH;
    glBegin(GL_QUADS);
    glVertex2s(0, 0);
    glVertex2s(w, 0);
    glVertex2s(w, h);
    glVertex2s(0, h);
    glEnd();
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
