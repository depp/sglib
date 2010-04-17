#ifndef WORLD_HPP
#define WORLD_HPP
class Object;

class World {
public:
    static const unsigned int kFrameTicks = 10;
    static const float kFrameTime;

    World();
    ~World();

    void addObject(Object *obj);
    void setPlayer(Object *obj);
    void draw();
    void update();

    float gameTime() const { return kFrameTime * frameNum_; }

private:
    void drawSky();
    void drawGround();

    World(const World &);
    World &operator=(const World &);

    Object *first_;
    Object *player_;
    float playerX_, playerY_, playerFace_;
    unsigned int frameNum_;
};

#endif
