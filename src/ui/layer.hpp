#ifndef UI_LAYER_HPP
#define UI_LAYER_HPP
#include "SDL.h"
namespace UI {

class Layer {
public:
    static Layer *front;

    virtual ~Layer();
    virtual void handleEvent(SDL_Event const &evt);
    virtual void draw();
};

}
#endif
