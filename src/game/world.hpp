#ifndef GAME_WORLD_HPP
#define GAME_WORLD_HPP
class Object;

class World {
public:
    static const unsigned int kFrameTicks = 10;
    static const float kFrameTime;
    static const unsigned int kMaxObjects = 256;

    World();
    ~World();

    void addObject(Object *obj);
    void setPlayer(Object *obj);
    void draw();
    void update();

    float gameTime() const { return kFrameTime * frameNum_; }
    void getCamera(float camera[3]);

private:
    void drawSky();
    void drawGround();

    World(const World &);
    World &operator=(const World &);

    unsigned int frameNum_, objCount_;
    Object *objects_[kMaxObjects], *player_;
    float playerX_, playerY_, playerFace_;
};

#endif
