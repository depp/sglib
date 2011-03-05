#include "game.hpp"
#include "menu.hpp"
#include "game/world.hpp"
#include "game/obstacle.hpp"
#include "graphics/model.hpp"
#include "event.hpp"
#include "SDL.h"

const unsigned int LAG_THRESHOLD = 1000;

UI::Game::~Game()
{
    if (world_)
        delete world_;
}

void UI::Game::handleEvent(UI::Event const &evt)
{
    switch (evt.type) {
    case KeyDown:
    case KeyUp:
        handleKey(evt.keyEvent());
        break;
    default:
        break;
    }
}

void UI::Game::handleKey(UI::KeyEvent const &evt)
{
    bool state = evt.type == KeyDown;
    switch (evt.key) {
    case SDLK_ESCAPE:
        setActive(new Menu);
        break;
    case SDLK_UP:
    case SDLK_w:
        input_.up = state;
        break;
    case SDLK_DOWN:
    case SDLK_s:
        input_.down = state;
        break;
    case SDLK_LEFT:
    case SDLK_a:
        input_.left = state;
        break;
    case SDLK_RIGHT:
    case SDLK_d:
        input_.right = state;
        break;
    case SDLK_SPACE:
        input_.fire = state;
        break;
    default:
        break;
    }
}

void UI::Game::draw(unsigned int ticks)
{
    unsigned int curfr, oldref;
    if (!world_) {
        tickref_ = ticks;
        initWorld();
    } else {
        unsigned int delta = ticks - tickref_, frames;
        if (delta > LAG_THRESHOLD) {
            world_->update();
            tickref_ = ticks;
        } else if (delta >= World::kFrameTicks) {
            frames = delta / World::kFrameTicks;
            tickref_ += frames * World::kFrameTicks;
            while (frames--)
                world_->update();
        }
    }
    world_->draw();
    curfr = framecount_;
    if (curfr == 64) {
        curfr = 0;
        havefps_ = true;
    }
    framecount_ = curfr + 1;
    if (havefps_) {
        oldref = frametick_[curfr];
        frametick_[curfr] = ticks;
        float rate = 64.0e3f / (ticks - oldref);
        float ms = (ticks - oldref) / 64.0f;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f FPS (%.1f ms)", rate, ms);
        if (!framerate_) {
            framerate_ = new RasterText;
            Font f;
            static char const *const family[] = {
                "Helvetica",
                "Arial",
                "DejaVu Sans",
                "Sans",
                NULL
            };
            f.setFamily(family);
            f.setSize(12.0f);
            framerate_->setFont(f);
        }
        framerate_->setText(buf);

        glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor3ub(128, 255, 128);
        glTranslatef(10.0f, 30.0f, 0.0f);
        framerate_->draw();
        glPopAttrib();
    }
}

void UI::Game::initWorld()
{
    world_ = new World;
    World &w = *world_;

    Object *obj;
    obj = new Obstacle(5.0f, 5.0f, 0.0f, 2.0f,
                       Model::kCube, Color::olive(), Color::yellow());
    w.addObject(obj);
    obj = new Obstacle(5.0f, -5.0f, 22.5f, 3.0f,
                       Model::kPyramid, Color::maroon(), Color::red());
    w.addObject(obj);
    obj = new Obstacle(-5.0f, 5.0f, 67.5f, 0.5f,
                       Model::kCube, Color::olive(), Color::yellow());
    w.addObject(obj);
    obj = new Obstacle(-5.0f, -5.0f, 45.0f, 1.5f,
                       Model::kPyramid, Color::maroon(), Color::red());
    w.addObject(obj);
    obj = new Player(0.0f, 0.0f, 0.0f, input_);
    w.addObject(obj);
    w.setPlayer(obj);
}
