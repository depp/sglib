#include "gamescreen.hpp"
#include "ui/menu.hpp"
#include "world.hpp"
#include "obstacle.hpp"
#include "graphics/model.hpp"
#include "ui/event.hpp"
namespace Tank {

GameScreen::~GameScreen()
{
    if (world_)
        delete world_;
}

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
        setActive(new UI::Menu);
        break;
    case UI::KUp:
        input_.up = state;
        break;
    case UI::KDown:
        input_.down = state;
        break;
    case UI::KLeft:
        input_.left = state;
        break;
    case UI::KRight:
        input_.right = state;
        break;
    case UI::KSelect:
        input_.fire = state;
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

        Tank::Object *obj;
        obj = new Tank::Obstacle(
            5.0f, 5.0f, 0.0f, 2.0f,
            Model::open("model/house.egg3d"),
            Color::olive(), Color::yellow());
        w.addObject(obj);
        obj = new Tank::Obstacle(
            5.0f, -5.0f, 22.5f, 3.0f,
            &Model::kPyramid, Color::maroon(), Color::red());
        w.addObject(obj);
        obj = new Tank::Obstacle(
            -5.0f, 5.0f, 67.5f, 0.5f,
            &Model::kCube, Color::olive(), Color::yellow());
        w.addObject(obj);
        obj = new Tank::Obstacle(
            -5.0f, -5.0f, 45.0f, 1.5f,
            &Model::kPyramid, Color::maroon(), Color::red());
        w.addObject(obj);
        obj = new Tank::Player(0.0f, 0.0f, 0.0f, input_);
        w.addObject(obj);
        w.setPlayer(obj);
    }
    world_->update(ticks);
}

void GameScreen::draw()
{
    world_->draw();
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
