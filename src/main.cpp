#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

SDL_Surface *screen = NULL;

void drawScene(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f, -0.7f, 0.0f);
    glVertex3f( 0.5f, -0.7f, 0.0f);
    glVertex3f( 0.0f,  0.7f, 0.0f);
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
