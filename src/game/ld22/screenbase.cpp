#include "defs.hpp"
#include "screenbase.hpp"
#include "area.hpp"
#include "actor.hpp"
#include "player.hpp"
#include "background.hpp"
#include "client/keyboard/keycode.h"
#include "client/opengl.hpp"
#include "client/ui/event.hpp"
#include "client/ui/window.hpp"
#include <cstdlib>
#include <stdio.h>
using namespace LD22;

ScreenBase::ScreenBase()
    :m_init(false)
{ }

ScreenBase::~ScreenBase()
{ }

static const unsigned LAG_THRESHOLD = 250;

void ScreenBase::advance()
{
    m_area->advance();
    m_background->advance();
}

void ScreenBase::init()
{
    m_area.reset(new Area);
    m_background.reset(Background::getBackground(Background::MOUNTAINS));
}

void ScreenBase::update(unsigned int ticks)
{
    if (!m_init) {
        init();
        if (!m_area.get() || !m_background.get())
            std::abort();
        m_tickref = ticks;
        m_init = true;
    } else {
        unsigned delta = ticks - m_tickref, frames;
        if (delta > LAG_THRESHOLD) {
            fputs("===== LAG =====", stderr);
            m_tickref = ticks;
            advance();
            m_delta = 0;
        } else {
            if (delta >= (unsigned) FRAME_TIME) {
                frames = delta / FRAME_TIME;
                m_tickref += frames * FRAME_TIME;
                while (frames--)
                    advance();
            }
            m_delta = delta % FRAME_TIME;
        }
    }
}

void ScreenBase::draw()
{
    if (!m_area.get() || !m_background.get())
        std::abort();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window().width(), 0, window().height(), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glScalef(32.0f, 32.0f, 1.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_background->draw(m_delta);
    m_area->draw(m_delta);
}
