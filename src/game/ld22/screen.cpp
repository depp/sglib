#include "defs.hpp"
#include "screen.hpp"
#include "area.hpp"
#include "player.hpp"
#include "client/keyboard/keycode.h"
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

Screen::Screen()
    : m_key(KEY_MAP)
{
    m_area.reset(new Area(*this));
}

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

void Screen::init()
{
    level().load(1);
    startGame();
}

void Screen::startGame()
{
    m_area->load();
    m_area->addActor(new Player(64, 64, *this));
}

void Screen::drawExtra(int delta)
{
    m_area->draw(delta);
}

void Screen::advance()
{
    m_area->advance();
}

