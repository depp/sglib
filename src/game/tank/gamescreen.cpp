#include "gamescreen.hpp"
#include "client/ui/menu.hpp"
#include "world.hpp"
#include "obstacle.hpp"
#include "client/model.hpp"
#include "client/ui/event.hpp"
#include "client/viewport.hpp"
#include "impl/kbd/keycode.h"
#include "player.hpp"
#include <string>
namespace Tank {

static const unsigned char KEY_MAP[] = {
    KEY_W, KeyForward,
    KP_8, KeyForward,
    KEY_Up, KeyForward,

    KEY_A, KeyLeft,
    KP_4, KeyLeft,
    KEY_Left, KeyLeft,

    KEY_S, KeyBack,
    KP_5, KeyBack,
    KEY_Down, KeyBack,
 
    KEY_D, KeyRight,
    KP_6, KeyRight,
    KEY_Right, KeyRight,

    KP_0, KeyFire,
    KEY_Space, KeyFire,

    255
};

GameScreen::GameScreen()
    : key_(KEY_MAP), world_(0)
{ }

GameScreen::~GameScreen()
{
    if (world_)
        delete world_;
}

void GameScreen::handleEvent(UI::Event const &evt)
{
    switch (evt.type) {
    case UI::KeyDown:
        if (evt.keyEvent().key == KEY_Escape) {
            makeActive(new UI::Menu);
            break;
        }
    case UI::KeyUp:
        key_.handleKeyEvent(evt.keyEvent());
        break;
    default:
        break;
    }
}

void GameScreen::update(unsigned int ticks)
{
    if (!world_) {
        world_ = new Tank::World;
        Tank::World &w = *world_;
        Model::Ref pyramid = Model::pyramid(), cube = Model::cube();

        Tank::Object *obj;
        obj = new Tank::Obstacle(
            5.0f, 5.0f, 0.0f, 2.0f,
            Model::file("model/house.egg3d"),
            Color::olive(), Color::yellow());
        w.addObject(obj);
        obj = new Tank::Obstacle(
            5.0f, -5.0f, 22.5f, 3.0f,
            pyramid, Color::maroon(), Color::red());
        w.addObject(obj);
        obj = new Tank::Obstacle(
            -5.0f, 5.0f, 67.5f, 0.5f,
            cube, Color::olive(), Color::yellow());
        w.addObject(obj);
        obj = new Tank::Obstacle(
            -5.0f, -5.0f, 45.0f, 1.5f,
            pyramid, Color::maroon(), Color::red());
        w.addObject(obj);
        obj = new Tank::Player(0.0f, 0.0f, 0.0f, key_);
        w.addObject(obj);
        w.setPlayer(obj);
    }
    world_->update(ticks);
}

void GameScreen::draw(Viewport &v, unsigned msec)
{
    (void) msec;
    world_->draw(v.width(), v.height());
    /*
    curfr = framecount_;
    if (curfr == 64) {
        curfr = 0;
        havefps_ = true;
    }
    framecount_ = curfr + 1;
    if (havefps_) {
        oldref = frametick_[curfr];
        frametick_[curfr] = framecur_;
        float rate = 64.0e3f / (framecur_ - oldref);
        float ms = (framecur_ - oldref) / 64.0f;
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
    */
}

}
