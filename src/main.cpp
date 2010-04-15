#include "SDL.h"
#include "SDL_opengl.h"
#include <cmath>
#include "world.hpp"
#include "cube.hpp"
#include "pyramid.hpp"

const float kPi = 4.0f * std::atan(1.0f);
const Uint32 kLagThreshold = 1000;
const Uint32 kFrameTime = 10;

const float kPlayerForwardSpeed = 10.0f * (kFrameTime * 0.001f);
const float kPlayerTurnSpeed = 100.0f * (kFrameTime * 0.001f);

const float kGridSpacing = 2.0f;
const int kGridSize = 8;

bool buttonLeft = false, buttonRight = false,
    buttonUp = false, buttonDown = false;
float playerX = 0.0f, playerY = 0.0f, playerFace = 0.0f;
Uint32 tickref = 0;

void init(void)
{
    SDL_Surface *screen = NULL;
    int flags;

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0) {
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
        buttonUp = state;
        break;
    case SDLK_DOWN:
    case SDLK_s:
        buttonDown = state;
        break;
    case SDLK_LEFT:
    case SDLK_a:
        buttonLeft = state;
        break;
    case SDLK_RIGHT:
    case SDLK_d:
        buttonRight = state;
        break;
    default:
        break;
    }
}

void handleEvents(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            handleKey(&event.key.keysym, true);
            break;
        case SDL_KEYUP:
            handleKey(&event.key.keysym, false);
            break;
        case SDL_QUIT:
            SDL_Quit();
            exit(0);
            break;
        }
    }
}

void advanceFrame(void)
{
    float forward = 0.0f, turn = 0.0f, face;
    if (buttonLeft)
        turn += kPlayerTurnSpeed;
    if (buttonRight)
        turn -= kPlayerTurnSpeed;
    if (buttonUp)
        forward += kPlayerForwardSpeed;
    if (buttonDown)
        forward -= kPlayerForwardSpeed;
    playerFace += turn;
    face = playerFace * (kPi / 180.0f);
    playerX += forward * std::cos(face);
    playerY += forward * std::sin(face);
}

void updateState(void)
{
    Uint32 tick = SDL_GetTicks();
    if (tick > tickref + kLagThreshold) {
        advanceFrame();
        tickref = tick;
    } else if (tick > tickref + kFrameTime) {
        do {
            advanceFrame();
            tickref += kFrameTime;
        } while (tick > tickref + kFrameTime);
    } else if (tick < tickref)
        tickref = tick;
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
    glPushAttrib(GL_CURRENT_BIT);
    glBegin(GL_TRIANGLE_STRIP);
    glColor3ubv(sky[0].color);
    glVertex3f(-2.0f, 1.0f, -1.0f);
    glVertex3f( 2.0f, 1.0f, -1.0f);
    for (int i = 0; i < sizeof(sky) / sizeof(*sky); ++i) {
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
    float px = std::floor(playerX / kGridSpacing + 0.5f) * kGridSpacing;
    float py = std::floor(playerY / kGridSpacing + 0.5f) * kGridSpacing;
    glPushAttrib(GL_CURRENT_BIT);
    glPushMatrix();
    glTranslatef(px, py, 0.0f);
    glScalef(kGridSpacing, kGridSpacing, 1.0f);
    glColor3ub(51, 0, 255);
    glBegin(GL_LINES);
    for (int i = -kGridSize; i <= kGridSize; ++i) {
        glVertex3f(i, -kGridSize, 0.0f);
        glVertex3f(i,  kGridSize, 0.0f);
        glVertex3f(-kGridSize, i, 0.0f);
        glVertex3f( kGridSize, i, 0.0f);
    }
    glEnd();
    glPopAttrib();
    glPopMatrix();
}

World world;

void drawScene(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.1f, 0.1f, -0.075f, 0.075f, 0.1f, 100.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawSky();

    glRotatef(90.0f - playerFace, 0.0f, 0.0f, 1.0f);
    glTranslatef(-playerX, -playerY, -1.0f);

    drawGround();

    world.draw();

    SDL_GL_SwapBuffers();
}

const unsigned char yellow[3] = { 255, 255, 0 };
const unsigned char red[3] = { 255, 51, 0 };

int main(int argc, char *argv[])
{
    init();
    world.addObject(new Cube( 5.0f,  5.0f, 2.0f, yellow));
    world.addObject(new Pyramid( 5.0f, -5.0f, 3.0f, red));
    world.addObject(new Cube(-5.0f,  5.0f, 0.5f, yellow));
    world.addObject(new Pyramid(-5.0f, -5.0f, 1.5f, red));
    while (1) {
        handleEvents();
        updateState();
        drawScene();
    }
    return 0;
}
