#ifndef TYPE_RASTERTEXT_HPP
#define TYPE_RASTERTEXT_HPP
#include <string>
#include "SDL_opengl.h"
#include "font.hpp"
#include "graphics/texture.hpp"

/* Rasterized text texture.  The texture is grayscale with no
   alpha.  */
class RasterText : public Texture {
public:
    typedef Texture::RefT<RasterText> Ref;
    static Ref create();

    enum Alignment {
        Left,
        Center,
        Right
    };

    virtual std::string name() const;

    void setText(std::string const &text);
    void setFont(Font const &font);
    void setAlignment(Alignment alignment);

    /* Loads or reloads the texture if necessary and draws the text at
       the coordinates (0,0).  */
    void draw();

protected:
    virtual bool load();

private:
    RasterText();
    virtual ~RasterText();

    std::string text_;
    Font font_;
    Alignment alignment_;
    float vx1_, vx2_, vy1_, vy2_, tx1_, tx2_, ty1_, ty2_;
};

#endif
