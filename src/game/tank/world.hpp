#ifndef GAME_TANK_WORLD_HPP
#define GAME_TANK_WORLD_HPP
namespace Tank {
class Object;

class World {
public:
    static const float kFrameTime;

    World();
    ~World();

    void addObject(Object *obj);
    void setPlayer(Object *obj);
    void draw(int w, int h);
    void update(unsigned int ticks);

    float gameTime() const { return kFrameTime * frameNum_; }
    void getCamera(float camera[3]);

private:
    static const unsigned int kMaxObjects = 256;

    void advanceFrame();
    void drawSky();
    void drawGround();

    World(const World &);
    World &operator=(const World &);

    bool initted_;
    unsigned int tickref_, frameNum_, objCount_;
    Object *objects_[kMaxObjects], *player_;
    float playerX_, playerY_, playerFace_;
};

}
#endif
