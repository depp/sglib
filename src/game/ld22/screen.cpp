#include "defs.hpp"
#include "screen.hpp"
#include "area.hpp"
#include "actor.hpp"
#include "player.hpp"
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

static const unsigned FRAME_TICKS = 32;
static const unsigned LAG_THRESHOLD = 250;

void Screen::advance()
{
    m_area->advance();
}

void Screen::update(unsigned int ticks)
{
    if (!m_area) {
        m_area = new Area;
        m_area->addActor(new Player(64, 64, *this));
        m_tickref = ticks;
    } else {
        unsigned delta = ticks - m_tickref, frames;
        if (delta > LAG_THRESHOLD) {
            m_tickref = ticks;
            advance();
        } else if (delta >= FRAME_TICKS) {
            frames = delta / FRAME_TICKS;
            m_tickref += frames * FRAME_TICKS;
            while (frames--)
                advance();
        }
    }
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
