#include "gamescreen.hpp"
#include "client/ui/menu.hpp"
#include "client/ui/event.hpp"
#include "client/ui/window.hpp"
#include "world.hpp"
#include "player.hpp"
#include "scenery.hpp"
namespace Space {

void GameScreen::handleEvent(UI::Event const &evt)
{
    switch (evt.type) {
    case UI::KeyDown:
    case UI::KeyUp:
        handleKey(evt.keyEvent());
        break;
    default:
        break;
    }
}

void GameScreen::handleKey(UI::KeyEvent const &evt)
{
    bool state = evt.type == UI::KeyDown;
    switch (evt.key) {
    case UI::KEscape:
        window().setScreen(new UI::Menu);
        break;

    case UI::KUp:
        world_->player()->setKey(KeyThrust, state);
        break;

    case UI::KDown:
        world_->player()->setKey(KeyBrake, state);
        break;

    case UI::KLeft:
        world_->player()->setKey(KeyLeft, state);
        break;

    case UI::KRight:
        world_->player()->setKey(KeyRight, state);
        break;

    case UI::KSelect:
        world_->player()->setKey(KeyFire, state);
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

        player_ = new Player();
        world_->addThinker(player_);
        world_->setPlayer(player_);
    }
    world_->update(ticks);
}

void GameScreen::draw()
{
    glClear(GL_COLOR_BUFFER_BIT);
    world_->draw(window().width(), window().height());
}

}
