#include "client/opengl.hpp"
#include "shot.hpp"
#include "entity.hpp"
#include "world.hpp"
namespace Space {

class Shot::ShotEntity : public Entity {
public:
    ShotEntity();
    virtual ~ShotEntity();

    virtual void move(World &w, double delta);
    virtual void draw();

    vector velocity;
};

Shot::Shot(vector const& location, vector const& velocity, double time)
{
    shot_ = new ShotEntity;
    shot_->location = location;
    shot_->velocity = velocity;
    endtime_ = time;
}

Shot::~Shot()
{ }

void Shot::think(World &w, double)
{
    if (w.time() > endtime_)
        w.removeThinker(this);
}

void Shot::enterGame(World &w)
{
    endtime_ += w.time();
    w.addEntity(shot_);
}

void Shot::leaveGame(World &w)
{
    w.removeEntity(shot_);
}

Shot::ShotEntity::ShotEntity()
{ }

Shot::ShotEntity::~ShotEntity()
{ }

void Shot::ShotEntity::move(World &, double delta)
{
    location += velocity * (float) delta;
}

void Shot::ShotEntity::draw()
{
    glColor3ub(0, 255, 255);
    glPointSize(2);
    glBegin(GL_POINTS);
    glVertex2f(location.v[0], location.v[1]);
    glEnd();
    glColor3ub(255, 255, 255);
}

}
