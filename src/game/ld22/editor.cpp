#include "editor.hpp"
#include "defs.hpp"
#include "client/opengl.hpp"
using namespace LD22;

static const int EDITBAR_SIZE = 64;

Editor::Editor()
{
    setSize(SCREEN_WIDTH + EDITBAR_SIZE, SCREEN_HEIGHT);
}

Editor::~Editor()
{ }

void Editor::handleEvent(const UI::Event &evt)
{
    
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
