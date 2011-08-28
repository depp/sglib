#ifndef GAME_SPACE_THINKER_HPP
#define GAME_SPACE_THINKER_HPP
namespace Space {
class World;

class Thinker {
public:
    virtual ~Thinker();
    virtual void think(World &w, double delta) = 0;
    virtual void enterGame(World &w);
    virtual void leaveGame(World &w);
};

}
#endif
