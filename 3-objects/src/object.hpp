#ifndef OBJECT_HPP
#define OBJECT_HPP
class World;

class Object {
    friend class World;
public:
    Object(float x, float y, float face, float size);
    virtual ~Object();
    virtual void draw() = 0;
    void setupMatrix();
    virtual void update();

    float getX() const { return x_; }
    float getY() const { return y_; }
    float getFace() const { return face_; }
    float getSpeed() const { return speed_; }
    float getSize() const { return size_; }
    void setFace(float f) { face_ = f; }
    void setSpeed(float s) { speed_ = s; }

private:
    Object(const Object &);
    Object &operator=(const Object &);

    float x_, y_, face_, size_, speed_;
    Object *next_, *prev_;
};

#endif
