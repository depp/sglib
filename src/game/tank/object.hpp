#ifndef GAME_OBJECT_HPP
#define GAME_OBJECT_HPP
class World;

class Object {
    friend class World;
public:
    static const unsigned int kClassSolid = 1 << 0;

    Object(unsigned int colGen, unsigned int colRcv,
           float x, float y, float face, float size);
    virtual ~Object();
    virtual void init();
    virtual void draw();
    void setupMatrix();
    virtual void update();
    virtual bool collide(Object &other);
    void remove() { index_ = -1; }

    float getX() const { return x_; }
    float getY() const { return y_; }
    float getFace() const { return face_; }
    float getSpeed() const { return speed_; }
    float getSize() const { return size_; }
    World &getWorld() { return *world_; }
    void setFace(float f) { face_ = f; }
    void setSpeed(float s) { speed_ = s; }

private:
    Object(const Object &);
    Object &operator=(const Object &);

    unsigned int colGen_, colRcv_;
    float x_, y_, face_, size_, speed_;
    int index_;
    World *world_;
};

#endif
