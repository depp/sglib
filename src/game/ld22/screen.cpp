#include "defs.hpp"
#include "screen.hpp"
#include "area.hpp"
#include "actor.hpp"
#include "client/keyboard/keycode.h"
#include "client/opengl.hpp"
#include "client/ui/event.hpp"
#include "client/ui/window.hpp"
#include <cstdlib>
using namespace LD22;

static const unsigned char KEY_MAP[] = {
    KEY_W, InUp,
    KP_8, InUp,
    KEY_Up, InUp,

    KEY_A, InLeft,
    KP_4, InLeft,
    KEY_Left, InLeft,

    KEY_S, InDown,
    KP_5, InDown,
    KEY_Down, InDown,
 
    KEY_D, InRight,
    KP_6, InRight,
    KEY_Right, InRight,

    255
};

Screen::Screen()
    : m_key(KEY_MAP), m_area(NULL)
{ }

Screen::~Screen()
{ }

void Screen::handleEvent(const UI::Event &evt)
{
    switch (evt.type) {
    case UI::KeyDown:
        // if (evt.keyEvent().key == KEY_Escape)
    case UI::KeyUp:
        m_key.handleKeyEvent(evt.keyEvent());
        break;

    default:
        break;
    }
}

void Screen::update(unsigned int ticks)
{
    if (!m_area) {
        m_area = new Area;
        m_area->addActor(new Actor(64, 64));
    }
    (void) ticks;
}

void Screen::draw()
{
    if (!m_area)
        std::abort();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window().width(), 0, window().height(), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glScalef(32.0f, 32.0f, 1.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_area->draw();
}
