#ifndef TYPE_TYPE_HPP
#define TYPE_TYPE_HPP
#include <string>
#include "SDL_opengl.h"

class Type {
public:
    Type();
    ~Type();

    /* Set the text drawn by the Type object.  */
    void setText(std::string const &text);

    /* Loads or reloads the texture if necessary and draws the text at
       the coordinates (0,0).  */
    void draw();

    /* Load or reload the texture if necessary.  */
    void load();

    /* Unload the texture if it is loaded.  */
    void unload();

private:
    /* Platform specific.  Allocates a buffer for a texture and
       renders the text into that buffer, returning the buffer and its
       dimensions.  The buffer is 8-bit grayscale, white text on a
       black background.  The dimensions are powers of two.  The
       vx/vy/tx/ty fields are initialized by this function.  This
       method does not use any OpenGL functions.  */
    void loadImage(void **data, unsigned int *width, unsigned int *height);

    std::string text_;
    bool textureLoaded_;
    GLuint texture_;
    float vx1_, vx2_, vy1_, vy2_, tx1_, tx2_, ty1_, ty2_;
};

#endif
