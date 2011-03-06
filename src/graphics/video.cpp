#include "video.hpp"
#include "ui/screen.hpp"
#include "SDL.h"
#include "SDL_opengl.h"
#include <stdio.h>

unsigned int Video::width = 0, Video::height = 0;

void Video::init()
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

void Video::update()
{
    SDL_GL_SwapBuffers();
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        const GLubyte *s = gluErrorString(err);
        fprintf(stderr, "GL Error: %s", s);
    }
}
