#include "shot.hpp"
#include "client/model.hpp"
#include "client/color.hpp"
#include "world.hpp"
#include "explosion.hpp"
#include "client/opengl.hpp"
namespace Tank {

static const float kShotTwist = 720.0f;
static const float kShotSpan = 1.0f;

Shot::Shot(float x, float y, float face)
    : Object(0, kClassSolid, x, y, face, 0.25), time_(0.0f)
{
    setSpeed(25.0f);
}

Shot::~Shot()
{ }

void Shot::init()
{
    time_ = getWorld().gameTime() + kShotSpan;
}

void Shot::draw()
{
    glPushMatrix();
    setupMatrix();
    glTranslatef(0.0f, 0.0f, 0.5f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(kShotTwist * getWorld().gameTime(), 0.0f, 0.0f, 1.0f);
    glScalef(0.125f, 0.125f, 0.5f);
    Model::pyramid()->draw(Color::green(), Color::lime());
    glPopMatrix();
}

void Shot::update()
{
    if (getWorld().gameTime() > time_)
        remove();
}

bool Shot::collide(Object &)
{
    remove();
    Object *obj = new Explosion(getX(), getY(), Explosion::kShot);
    getWorld().addObject(obj);
    return false;
}

}
