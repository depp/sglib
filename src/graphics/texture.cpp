#include "texture.hpp"
#include "SDL_opengl.h"

Texture::NullSource Texture::nullSrc;

void Texture::NullSource::load()
{
    unsigned char buf[16][16][3];
    unsigned int x, y;
    GLuint tex = 0;
    for (y = 0; y < 8; ++y) {
        for (x = 0; x < 8; ++x) {
            buf[y+0][x+0][0] = 255;
            buf[y+0][x+0][1] = 0;
            buf[y+0][x+0][2] = 255;
            buf[y+0][x+8][0] = 32;
            buf[y+0][x+8][1] = 32;
            buf[y+0][x+8][2] = 32;
            buf[y+8][x+0][0] = 32;
            buf[y+8][x+0][1] = 32;
            buf[y+8][x+0][2] = 32;
            buf[y+8][x+8][0] = 255;
            buf[y+8][x+8][1] = 0;
            buf[y+8][x+8][2] = 255;
        }
    }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 16, 16, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, buf);
    tex_ = tex;
}
