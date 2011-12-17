#include "defs.hpp"
#include "screen.hpp"
#include "client/keyboard/keycode.h"
#include "client/opengl.hpp"
#include "client/ui/event.hpp"
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

LD22::Screen::Screen()
    : key_(KEY_MAP)
{ }

LD22::Screen::~Screen()
{ }

void LD22::Screen::handleEvent(const UI::Event &evt)
{
    switch (evt.type) {
    case UI::KeyDown:
        // if (evt.keyEvent().key == KEY_Escape)
    case UI::KeyUp:
        key_.handleKeyEvent(evt.keyEvent());
        break;

    default:
        break;
    }
}

void LD22::Screen::update(unsigned int ticks)
{
    (void) ticks;
}

void LD22::Screen::draw()
{
    glClearColor(1.0f, 0.5f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
