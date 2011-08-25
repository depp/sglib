#ifndef SHOT_H
#define SHOT_H
#include "thinker.hpp"
namespace Space {
class vector;

class Shot : public Thinker {
    class ShotEntity;
    ShotEntity *shot_;
    double endtime_;

public:
    Shot(vector const& location, vector const& velocity, double time);
    virtual ~Shot();

    virtual void think(World &w, double delta);
    virtual void enterGame(World &w);
    virtual void leaveGame(World &w);
};

} // namespace sparks
#endif
