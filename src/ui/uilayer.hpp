#ifndef UI_UILAYER_HPP
#define UI_UILAYER_HPP
#include "SDL.h"

class UILayer {
public:
    static UILayer *front;

    virtual ~UILayer();
    virtual void handleEvent(SDL_Event const &evt);
    virtual void draw();
};

#endif
