#include "SDL.h"
// #include "SDL_opengl.h"
// #include <cmath>
// #include "game/world.hpp"
// #include "game/obstacle.hpp"
// #include "graphics/model.hpp"
// #include "game/player.hpp"
#include "rand.hpp"
#include "ui/menu.hpp"
#include "ui/event.hpp"
#include "graphics/video.hpp"

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "Could not initialize SDL Timer: %s\n",
                SDL_GetError());
        exit(1);
    }
    Video::init();
    Rand::global.seed();
    UI::Screen::active = new UI::Menu();

    while (1) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (!UI::Screen::active)
                goto quit;
            switch (e.type) {
            case SDL_QUIT:
                goto quit;
            case SDL_MOUSEMOTION:
            {
                SDL_MouseMotionEvent &m = e.motion;
                int x = m.x, y = Video::height - 1 - m.y;
                UI::Screen::active->
                    handleEvent(UI::MouseEvent(UI::MouseMove, -1, x, y));
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                SDL_MouseButtonEvent &m = e.button;
                UI::EventType t = e.type == SDL_MOUSEBUTTONDOWN ?
                    UI::MouseDown : UI::MouseUp;
                int button;
                switch (m.button) {
                case SDL_BUTTON_LEFT:
                    button = UI::ButtonLeft;
                    break;
                case SDL_BUTTON_MIDDLE:
                    button = UI::ButtonMiddle;
                    break;
                case SDL_BUTTON_RIGHT:
                    button = UI::ButtonRight;
                    break;
                default:
                    button = UI::ButtonOther + m.button - 4;
                    break;
                }
                int x = m.x, y = Video::height - 1 - m.y;
                UI::Screen::active->
                    handleEvent(UI::MouseEvent(t, button, x, y));
                if (t == UI::MouseUp) {
                    /* Deliver a mouse movement event, this ensures
                       that highlighting will work correctly if a
                       widget has been capturing mouse events.  */
                    if (!UI::Screen::active)
                        goto quit;
                    UI::Screen::active->
                        handleEvent(UI::MouseEvent(UI::MouseMove, -1, x, y));
                }
                break;
            }
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                SDL_KeyboardEvent &k = e.key;
                UI::EventType t = e.type == SDL_KEYDOWN ?
                    UI::KeyDown : UI::KeyUp;
                UI::Screen::active->
                    handleEvent(UI::KeyEvent(t, k.keysym.sym));
                break;
            }
            default:
                break;
            }
        }
        if (!UI::Screen::active)
            goto quit;
        Video::draw();
    }

quit:
    SDL_Quit();
    return 0;
}