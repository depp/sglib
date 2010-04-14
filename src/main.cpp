#include "SDL.h"
#include "SDL_opengl.h"
#include <cmath>

const float kPi = 4.0f * std::atan(1.0f);
const float kPlayerForwardSpeed = 0.4f;
const float kPlayerTurnSpeed = 10.0f;
float playerX = 0.0f, playerY = 0.0f, playerFace = 0.0f;

float playerMove(float d)
{
    playerX += d * cos(playerFace * (kPi / 180.0f));
    playerY += d * sin(playerFace * (kPi / 180.0f));
}

float playerTurn(float a)
{
    playerFace += a;
    if (playerFace >= 360.0f)
        playerFace -= 360.0f;
    else if (playerFace < 0.0f)
        playerFace += 360.0f;
}

void drawScene(void)
{
    int x, y;
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.1f, 0.1f, -0.075f, 0.075f, 0.1f, 100.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(90.0f - playerFace, 0.0f, 0.0f, 1.0f);
    glTranslatef(-playerX, -playerY, -1.0f);
    glMatrixMode(GL_MODELVIEW);

    glBegin(GL_TRIANGLES);
    for (x = -5; x <= 5; ++x) {
        for (y = -5; y <= 5; ++y) {
            if (x == 1 && y == 0)
                glColor3f(0.0f, 1.0f, 0.0f);
            else
                glColor3f(1.0f, 0.0f, 0.0f);
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
    case SDLK_UP:
    case SDLK_w:
        playerMove(kPlayerForwardSpeed);
        break;
    case SDLK_DOWN:
    case SDLK_s:
        playerMove(-kPlayerForwardSpeed);
        break;
    case SDLK_LEFT:
    case SDLK_a:
        playerTurn(kPlayerTurnSpeed);
        break;
    case SDLK_RIGHT:
    case SDLK_d:
        playerTurn(-kPlayerTurnSpeed);
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
