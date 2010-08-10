#include "uilayer.hpp"

UILayer *UILayer::front = NULL;

UILayer::~UILayer()
{ }

void UILayer::handleEvent(SDL_Event const &evt)
{ }

void UILayer::draw()
{ }
