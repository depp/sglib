
#if 0
#ifndef CLIENT_TYPE_RASTERTEXT_HPP
#define CLIENT_TYPE_RASTERTEXT_HPP
#include <string>
#include "client/opengl.hpp"
#include "font.hpp"
#include "client/texture.hpp"

/* Rasterized text texture.  The texture is grayscale with no
   alpha.  */
class RasterText : public Texture {
    template<class T> friend class SharedRef;

private:
    

    typedef SharedRef<RasterText> Ref;
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
    virtual bool loadTexture();

private:
    RasterText();
    virtual ~RasterText();

    std::string text_;
    Font font_;
    Alignment alignment_;
    float vx1_, vx2_, vy1_, vy2_, tx1_, tx2_, ty1_, ty2_;
};

#endif
#endif
