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

private:
    Object(const Object &);
    Object &operator=(const Object &);

    float x_, y_, face_;
    Object *next_, *prev_;
};

#endif
