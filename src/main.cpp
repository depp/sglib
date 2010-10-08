#include "SDL.h"
#include "SDL_opengl.h"
#include <cmath>
#include "game/world.hpp"
#include "game/obstacle.hpp"
#include "graphics/model.hpp"
#include "game/player.hpp"
#include "rand.hpp"
#include "ui/menu.hpp"
#include "ui/event.hpp"
#include "graphics/video.hpp"

const Uint32 kLagThreshold = 1000;
Uint32 tickref = 0;
Player::Input input = { false, false, false, false, false };
World world;

void init(void)
{
    if (SDL_Init(SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "Could not initialize SDL Timer: %s\n",
                SDL_GetError());
        exit(1);
    }
    Video::init();

    Object *obj;
    obj = new Obstacle(5.0f, 5.0f, 0.0f, 2.0f,
                       Model::kCube, Color::olive(), Color::yellow());
    world.addObject(obj);
    obj = new Obstacle(5.0f, -5.0f, 22.5f, 3.0f,
                       Model::kPyramid, Color::maroon(), Color::red());
    world.addObject(obj);
    obj = new Obstacle(-5.0f, 5.0f, 67.5f, 0.5f,
                       Model::kCube, Color::olive(), Color::yellow());
    world.addObject(obj);
    obj = new Obstacle(-5.0f, -5.0f, 45.0f, 1.5f,
                       Model::kPyramid, Color::maroon(), Color::red());
    world.addObject(obj);
    obj = new Player(0.0f, 0.0f, 0.0f, input);
    world.addObject(obj);
    world.setPlayer(obj);

    Rand::global.seed();
}

void handleKey(SDL_keysym *key, bool state)
{
    switch (key->sym) {
    case SDLK_ESCAPE:
        if (state) {
            SDL_Quit();
            exit(0);
        }
        break;
    case SDLK_UP:
    case SDLK_w:
        input.up = state;
        break;
    case SDLK_DOWN:
    case SDLK_s:
        input.down = state;
        break;
    case SDLK_LEFT:
    case SDLK_a:
        input.left = state;
        break;
    case SDLK_RIGHT:
    case SDLK_d:
        input.right = state;
        break;
    case SDLK_SPACE:
        input.fire = state;
        break;
    default:
        break;
    }
}

void updateState(void)
{
    Uint32 tick = SDL_GetTicks();
    if (tick > tickref + kLagThreshold) {
        world.update();
        tickref = tick;
    } else if (tick > tickref + World::kFrameTicks) {
        do {
            world.update();
            tickref += World::kFrameTicks;
        } while (tick > tickref + World::kFrameTicks);
    } else if (tick < tickref)
        tickref = tick;
}

int main(int argc, char *argv[])
{
    init();
    UI::Screen::active = new UI::Menu();
    while (1) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            UI::Screen *s = UI::Screen::active;
            switch (e.type) {
            case SDL_QUIT:
                SDL_Quit();
                return 0;
            case SDL_MOUSEMOTION:
            {
                SDL_MouseMotionEvent &m = e.motion;
                int x = m.x, y = Video::height - 1 - m.y;
                s->handleEvent(UI::MouseEvent(UI::MouseMove, -1, x, y));
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
                s->handleEvent(UI::MouseEvent(t, button, x, y));
                break;
            }
            default:
                break;
            }
        }
        Video::draw();
    }
    return 0;
}
