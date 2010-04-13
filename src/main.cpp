#include "SDL.h"
#include "SDL_opengl.h"
#include <cmath>

void drawScene(void)
{
    int x, y;
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.1f, 0.1f, -0.075f, 0.075f, 0.1f, 100.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);

    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    for (x = -5; x <= 5; ++x) {
        for (y = -5; y <= 5; ++y) {
            glVertex3f(2*x - 0.5f, 2*y - 0.7f, 0.0f);
            glVertex3f(2*x + 0.5f, 2*y - 0.7f, 0.0f);
            glVertex3f(2*x + 0.0f, 2*y + 0.7f, 0.0f);
        }
    }
    glEnd();

    SDL_GL_SwapBuffers();
}

void handleKey(SDL_keysym *key)
{
    switch (key->sym) {
    case SDLK_ESCAPE:
        SDL_Quit();
        exit(0);
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    SDL_Surface *screen = NULL;
    bool done = false;
    SDL_Event event;
    int flags;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n",
                SDL_GetError());
        exit(1);
    }
    flags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER;
    screen = SDL_SetVideoMode(640, 480, 32, flags);
    if (!screen) {
        fprintf(stderr, "Could not initialize video: %s\n",
                SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    while (!done) {
        drawScene();
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                handleKey(&event.key.keysym);
                break;
            case SDL_QUIT:
                done = true;
                break;
            }
        }
    }
    SDL_Quit();
    return 0;
}
