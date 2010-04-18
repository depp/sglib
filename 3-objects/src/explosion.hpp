#ifndef EXPLOSION_HPP
#define EXPLOSION_HPP
#include "object.hpp"

class Explosion : public Object {
public:
    struct Type;
    static const Type kShot;

    Explosion(float x, float y, const Type &type);
    virtual ~Explosion();
    virtual void init();
    virtual void draw();
    virtual void update();

private:
    Explosion(const Explosion &);
    Explosion &operator=(const Explosion &);

    const Type &type_;
    float time_;
    short (*vertex_)[3];
};

#endif
