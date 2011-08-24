#include "SDL.h"
#include "rand.hpp"
#include "ui/menu.hpp"
#include "ui/event.hpp"
#include "ui/screen.hpp"
#include "graphics/video.hpp"
#include "opengl.hpp"
#include "sys/resource.hpp"
#include "sys/config.hpp"
#include "sys/path.hpp"
#include <stdio.h>

static const unsigned int MAX_FPS = 100;
static const unsigned int MIN_FRAMETIME = 1000 / MAX_FPS;

static void videoInit()
{
    SDL_Surface *screen = NULL;
    int flags;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL Video: %s\n",
                SDL_GetError());
        exit(1);
    }
    flags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER;
    screen = SDL_SetVideoMode(768, 480, 32, flags);
    if (!screen) {
        fprintf(stderr, "Could not initialize video: %s\n",
                SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    printf("Vendor: %s\nRenderer: %s\nVersion: %s\n",
           glGetString(GL_VENDOR), glGetString(GL_RENDERER),
           glGetString(GL_VERSION));

    Video::width = 768;
    Video::height = 480;
}

static void videoUpdate()
{
    SDL_GL_SwapBuffers();
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        const GLubyte *s = gluErrorString(err);
        fprintf(stderr, "GL Error: %s", s);
    }
}

static int mapKey(int key)
{
    switch (key) {
    case SDLK_RETURN:
    case SDLK_SPACE:
    case SDLK_KP_ENTER:
        return UI::KSelect;

    case SDLK_ESCAPE:
        return UI::KEscape;

    case SDLK_KP4:
    case SDLK_a:
    case SDLK_LEFT:
        return UI::KLeft;

    case SDLK_KP8:
    case SDLK_w:
    case SDLK_UP:
        return UI::KUp;

    case SDLK_KP6:
    case SDLK_d:
    case SDLK_RIGHT:
        return UI::KRight;

    case SDLK_KP5:
    case SDLK_KP2:
    case SDLK_s:
    case SDLK_DOWN:
        return UI::KDown;

    default:
        return -1;
    }
}

int main(int, char *[])
{
    Path::init();
    if (SDL_Init(SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "Could not initialize SDL Timer: %s\n",
                SDL_GetError());
        exit(1);
    }
    videoInit();
    Rand::global.seed();
    UI::Screen::setActive(new UI::Menu);

    unsigned int lastticks = SDL_GetTicks();
    while (1) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (!UI::Screen::getActive())
                goto quit;
            switch (e.type) {
            case SDL_QUIT:
                goto quit;
            case SDL_MOUSEMOTION:
            {
                SDL_MouseMotionEvent &m = e.motion;
                int x = m.x, y = Video::height - 1 - m.y;
                UI::Screen::getActive()->
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
                UI::Screen::getActive()->
                    handleEvent(UI::MouseEvent(t, button, x, y));
                break;
            }
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                SDL_KeyboardEvent &k = e.key;
                UI::EventType t = e.type == SDL_KEYDOWN ?
                    UI::KeyDown : UI::KeyUp;
                UI::Screen::getActive()->
                    handleEvent(UI::KeyEvent(t, mapKey(k.keysym.sym)));
                break;
            }
            default:
                break;
            }
        }
        if (!UI::Screen::getActive())
            goto quit;
        unsigned int ticks = SDL_GetTicks(), delta = ticks - lastticks;
        if (delta < MIN_FRAMETIME) {
            SDL_Delay(MIN_FRAMETIME - delta);
            lastticks += MIN_FRAMETIME;
            ticks = SDL_GetTicks();
        } else
            lastticks = ticks;
        UI::Screen *s = UI::Screen::getActive();
        s->update(ticks);
        Resource::loadAll();
        s->draw();
        videoUpdate();
    }

quit:
    SDL_Quit();
    return 0;
}
