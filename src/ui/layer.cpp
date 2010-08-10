#include "layer.hpp"

UI::Layer *UI::Layer::front = NULL;

UI::Layer::~Layer()
{ }

void UI::Layer::handleEvent(SDL_Event const &evt)
{ }

void UI::Layer::draw()
{ }
