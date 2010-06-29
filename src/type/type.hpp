#ifndef TYPE_HPP
#define TYPE_HPP
#include <string>
#include "SDL_opengl.h"

class Type {
public:
    Type();
    ~Type();

    void setText(std::string const &text);
    void draw();
    void load();
    void unload();

private:
    std::string text_;
    bool textureLoaded_;
    GLuint texture_;
    float vx1_, vx2_, vy1_, vy2_, tx1_, tx2_, ty1_, ty2_;
};

#endif
