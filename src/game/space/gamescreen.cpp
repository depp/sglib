#include "gamescreen.hpp"
#include "ui/menu.hpp"
#include "ui/event.hpp"
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
    // bool state = evt.type == UI::KeyDown;
    switch (evt.key) {
    case UI::KEscape:
        setActive(new UI::Menu);
        break;
    case UI::KUp:
        
        break;
    case UI::KDown:
        break;
    case UI::KLeft:
        break;
    case UI::KRight:
        break;
    case UI::KSelect:
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
    world_->draw();
}

}
