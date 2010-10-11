#include "screen.hpp"

UI::Screen *UI::Screen::active = 0;

void UI::Screen::setActive(Screen *screen)
{
    if (active != screen) {
        if (active)
            delete active;
        active = screen;
    }
}

UI::Screen::~Screen()
{ }

void UI::Screen::handleEvent(Event const &evt)
{ }

void UI::Screen::draw(unsigned int ticks)
{ }
