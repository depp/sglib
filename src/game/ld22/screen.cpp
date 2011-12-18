#include "defs.hpp"
#include "screen.hpp"
#include "area.hpp"
#include "client/keyboard/keycode.h"
#include "client/ui/event.hpp"
using namespace LD22;

static const int TRANSITION_TIME = 32 * 1;

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

    KEY_Space, InThrow,
    KP_0, InThrow,

    255
};

Screen::Screen()
    : m_key(KEY_MAP), m_levelno(0)
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
    startLevel(1);
}

static void drawFade(int t, int r, int g, int b, int a, int maxt)
{
    int aa;
    if (t < 0) aa = 0;
    else if (t >= maxt) aa = a;
    else aa = t * a / maxt;
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(r, g, b, aa);
    glBegin(GL_QUADS);
    glVertex2s(0, 0);
    glVertex2s(SCREEN_WIDTH, 0);
    glVertex2s(SCREEN_WIDTH, SCREEN_HEIGHT);
    glVertex2s(0, SCREEN_HEIGHT);
    glEnd();
    glDisable(GL_BLEND);
    glColor3ub(255, 255, 255);
}

void Screen::drawExtra(int delta)
{
    m_area->draw(delta);
    switch (m_state) {
    case SPlay:
        break;

    case SLose:
        drawFade(m_timer, 200, 0, 0, 110, 15);
        break;

    case SWin:
        drawFade(m_timer, 200, 200, 200, 150, 15);
        break;
    }
}

void Screen::advance()
{
    ScreenBase::advance();
    m_area->advance();
    m_timer++;
    switch (m_state) {
    case SLose:
        if (m_timer == TRANSITION_TIME)
            startLevel(m_levelno);
        break;

    case SWin:
        if (m_timer == TRANSITION_TIME)
            startLevel(m_levelno + 1);
        break;

    default:
        break;
    }
}

void Screen::lose()
{
    if (m_state == SPlay) {
        m_state = SLose;
        m_timer = 0;
    }
}

void Screen::win()
{
    if (m_state == SPlay) {
        m_state = SWin;
        m_timer = 0;
    }
}

void Screen::startLevel(int num)
{
    if (m_levelno != num)
        level().load(num);
    loadLevel();
    m_area->load();
    m_state = SPlay;
    m_timer = 0;
    m_levelno = num;
}
