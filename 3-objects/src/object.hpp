#ifndef OBJECT_HPP
#define OBJECT_HPP
class World;

class Object {
    friend class World;
public:
    Object(float x, float y, float face);
    virtual ~Object();
    virtual void draw() = 0;
    void setupMatrix();
    virtual void update();

    float getX() { return x_; }
    float getY() { return y_; }
    float getFace() { return face_; }
    float getSpeed() { return speed_; }
    void setFace(float f) { face_ = f; }
    void setSpeed(float s) { speed_ = s; }

private:
    Object(const Object &);
    Object &operator=(const Object &);

    float x_, y_, face_, speed_;
    Object *next_, *prev_;
};

#endif
