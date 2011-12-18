#include "defs.hpp"
#include "screen.hpp"
#include "area.hpp"
#include "client/keyboard/keycode.h"
#include "client/ui/event.hpp"
#include "client/texturefile.hpp"
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
    m_passfail1 = TextureFile::open("menu/passfail1.jpg");
    m_passfail2 = TextureFile::open("menu/passfail2.jpg");
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

static const unsigned char FADE_LOSE[5][4] = {
    { 0, 0, 0, 0 },
    { 200, 0, 0, 110 },
    { 100, 0, 0, 255 },
    { 0, 0, 0, 110 },
    { 200, 0, 0, 0 }
};

static const unsigned char FADE_WIN[5][4] = {
    { 0, 0, 0, 0 },
    { 255, 255, 255, 150 },
    { 150, 175, 200, 255 },
    { 0, 0, 0, 150 },
    { 255, 255, 255, 0 }
};

void Screen::drawExtra(int delta)
{
    bool win = false;
    const unsigned char (*c)[4] = NULL;
    unsigned char cc[4];
    int idx, frac, picfrac = 0;
    m_area->draw(delta);
    switch (m_state) {
    case SPlay:
        return;

    case SLose:
        win = false;
        c = FADE_LOSE;
        break;

    case SWin:
        win = true;
        c = FADE_WIN;
        break;
    }

    int t = m_timer, dt;
    if (t < 15) {
        dt = 15;
        idx = 0;
    } else if (t < 30) {
        t -= 15;
        dt = 15;
        idx = 1;
    } else if (t < 35) {
        t -= 30;
        dt = 5;
        idx = 2;
    } else if (t < 40) {
        t -= 35;
        dt = 5;
        idx = 3;
    } else {
        return;
    }
    frac = 256 * (t * FRAME_TIME + delta) / (dt * FRAME_TIME);
    for (int i = 0; i < 4; ++i) {
        int a = c[idx][i], b = c[idx+1][i];
        cc[i] = (b * frac + a * (256 - frac)) / 256;
    }
    switch (idx) {
    case 0: picfrac = frac; break;
    case 1: picfrac = 256; break;
    case 2: picfrac = 256 - frac; break;
    case 3: picfrac = 0;
    }

    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(cc[0], cc[1], cc[2], cc[3]);
    glBegin(GL_QUADS);
    glVertex2s(0, 0);
    glVertex2s(SCREEN_WIDTH, 0);
    glVertex2s(SCREEN_WIDTH, SCREEN_HEIGHT);
    glVertex2s(0, SCREEN_HEIGHT);
    glEnd();
    glColor3ub(255, 255, 255);

    if (picfrac) {
        static const int ICONSIZE = 256;
        int x0 = (SCREEN_WIDTH - ICONSIZE) / 2;
        int x1 = x0 + ICONSIZE;
        int y0 = (SCREEN_HEIGHT - ICONSIZE) / 2;
        int y1 = y0 + ICONSIZE;
        float u0 = win ? 0.0f : 0.5f, u1 = u0 + 0.5f;
        float v0 = 1.0f, v1 = 0.0f;
        glEnable(GL_TEXTURE_2D);
        if (picfrac > 128)
            picfrac -= 1;
        glColor3ub(picfrac, picfrac, picfrac);

        m_passfail2->bind();
        glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex2s(x0, y0);
        glTexCoord2f(u0, v1); glVertex2s(x0, y1);
        glTexCoord2f(u1, v1); glVertex2s(x1, y1);
        glTexCoord2f(u1, v0); glVertex2s(x1, y0);
        glEnd();

        m_passfail1->bind();
        glBlendFunc(GL_ONE, GL_ONE);
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex2s(x0, y0);
        glTexCoord2f(u0, v1); glVertex2s(x0, y1);
        glTexCoord2f(u1, v1); glVertex2s(x1, y1);
        glTexCoord2f(u1, v0); glVertex2s(x1, y0);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    }
    glColor3ub(255, 255, 255);
    glDisable(GL_BLEND);
}

void Screen::advance()
{
    bool win = false;
    ScreenBase::advance();
    switch (m_state) {
    case SPlay:
        m_area->advance();
        return;

    case SLose:
        win = false;
        break;

    case SWin:
        win = true;
        break;
    }
    m_timer++;
    if (m_timer < 30)
        m_area->advance();
    if (m_timer == 30)
        startLevel(m_levelno + (win ? 1 : 0));
    if (m_timer == 40)
        m_state = SPlay;
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
