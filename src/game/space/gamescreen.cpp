#include "gamescreen.hpp"
#include "client/ui/menu.hpp"
#include "client/ui/event.hpp"
#include "client/viewport.hpp"
#include "client/keycode.hpp"
#include "world.hpp"
#include "player.hpp"
#include "scenery.hpp"
namespace Space {

static const unsigned char KEY_MAP[] = {
    KEY_W, KeyThrust,
    KP_8, KeyThrust,
    KEY_Up, KeyThrust,

    KEY_A, KeyLeft,
    KP_4, KeyLeft,
    KEY_Left, KeyLeft,

    KEY_S, KeyBrake,
    KP_5, KeyBrake,
    KEY_Down, KeyBrake,

    KEY_D, KeyRight,
    KP_6, KeyRight,
    KEY_Right, KeyRight,

    KP_0, KeyFire,
    KEY_Space, KeyFire,

    255
};

GameScreen::GameScreen()
    : kmgr_(KEY_MAP), world_(0)
{ }

void GameScreen::handleEvent(UI::Event const &evt)
{
    switch (evt.type) {
    case UI::KeyDown:
        if (evt.keyEvent().key == KEY_Escape) {
            makeActive(new UI::Menu);
            break;
        }
    case UI::KeyUp:
        kmgr_.handleKeyEvent(evt.keyEvent());
        break;
    default:
        break;
    }
}

void GameScreen::update(unsigned int ticks)
{
    if (!world_) {
        world_ = new World;

        Scenery *s = new Scenery;
        s->radius = 32.0f;
        world_->addEntity(s);

        player_ = new Player(kmgr_);
        world_->addThinker(player_);
        world_->setPlayer(player_);
    }
    world_->update(ticks);
}

void GameScreen::draw(Viewport &v, unsigned msec)
{
    (void) msec;
    glClear(GL_COLOR_BUFFER_BIT);
    world_->draw(v.width(), v.height());
}

}
