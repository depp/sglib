#ifndef THINKER_H
#define THINKER_H
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
