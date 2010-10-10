#include "screen.hpp"

UI::Screen *UI::Screen::active = 0;

UI::Screen::~Screen()
{ }

void UI::Screen::handleEvent(Event const &evt)
{ }

void UI::Screen::draw(unsigned int ticks)
{ }
