#include "SDL.h"
#include "SDL_opengl.h"
#include <cmath>

const float kPi = 4.0f * std::atan(1.0f);
const float kPlayerForwardSpeed = 0.4f;
const float kPlayerTurnSpeed = 10.0f;
float playerX = 0.0f, playerY = 0.0f, playerFace = 0.0f;

const float kGridSpacing = 2.0f;
const int kGridSize = 8;

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

struct gradient_point {
    float pos;
    unsigned char color[3];
};

const gradient_point sky[] = {
    { -0.50f, {   0,   0,  51 } },
    { -0.02f, {   0,   0,   0 } },
    {  0.00f, { 102, 204, 255 } },
    {  0.20f, {  51,   0, 255 } },
    {  0.70f, {   0,   0,   0 } }
};

void drawSky(void)
{
	int i;
    glPushAttrib(GL_CURRENT_BIT);
    glBegin(GL_TRIANGLE_STRIP);
    glColor3ubv(sky[0].color);
    glVertex3f(-2.0f, 1.0f, -1.0f);
    glVertex3f( 2.0f, 1.0f, -1.0f);
    for (i = 0; i < sizeof(sky) / sizeof(*sky); ++i) {
        glColor3ubv(sky[i].color);
        glVertex3f(-2.0f, 1.0f, sky[i].pos);
        glVertex3f( 2.0f, 1.0f, sky[i].pos);
    }
    glVertex3f(-2.0f, 0.0f, 1.0f);
    glVertex3f( 2.0f, 0.0f, 1.0f);
    glEnd();
    glPopAttrib();
}

void drawGround(void)
{
    int i;
    glColor3ub(51, 0, 255);
    glBegin(GL_LINES);
    for (i = -kGridSize; i <= kGridSize; ++i) {
        glVertex3f(i * kGridSpacing, -kGridSize * kGridSpacing, 0.0f);
        glVertex3f(i * kGridSpacing,  kGridSize * kGridSpacing, 0.0f);
        glVertex3f(-kGridSize * kGridSpacing, i * kGridSpacing, 0.0f);
        glVertex3f( kGridSize * kGridSpacing, i * kGridSpacing, 0.0f);
    }
    glEnd();
}

void drawScene(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.1f, 0.1f, -0.075f, 0.075f, 0.1f, 100.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    drawSky();
    glRotatef(90.0f - playerFace, 0.0f, 0.0f, 1.0f);
    glTranslatef(-playerX, -playerY, -1.0f);
    glMatrixMode(GL_MODELVIEW);
    drawGround();
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

    printf(
        "Vendor: %s\n"
        "Renderer: %s\n"
        "Version: %s\n"
        "Extensions: %s\n",
        glGetString(GL_VENDOR), glGetString(GL_RENDERER),
        glGetString(GL_VERSION), glGetString(GL_EXTENSIONS));

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
