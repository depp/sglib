#ifndef GAME_SPACE_WORLD_HPP
#define GAME_SPACE_WORLD_HPP
#include <vector>
#include <set>
namespace Space {
class Entity;
class Thinker;
class Starfield;
class Player;

class World {
public:
    World();
    ~World();

    void draw();
    void update(unsigned ticks);

    void addEntity(Entity *e);
    void removeEntity(Entity *e);
    void addThinker(Thinker *t);
    void removeThinker(Thinker *t);

    Player *player()
    {
        return player_;
    }

    void setPlayer(Player *p)
    {
        player_ = p;
    }

    double time()
    {
        return time_;
    }

private:
    struct Event;

    Player *player_;
    double time_;
    std::set<Entity*> entities_;
    std::set<Thinker*> thinkers_;
    std::vector<Event> events_;
    std::vector<Starfield> starfields_;
};

}
#endif
