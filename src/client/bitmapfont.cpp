#include "bitmapfont.hpp"
#include "texturefile.hpp"

BitmapFont::BitmapFont(const std::string &path)
{
    m_tex = TextureFile::open(path);
}

BitmapFont::~BitmapFont()
{ }


void BitmapFont::print(int x, int y, const char *text)
{
    int xp = x, yp = y;
    int xs = m_tex->width() / 16, ys = m_tex->height() / 16;
    float xt = (float) xs / m_tex->twidth();
    float yt = (float) ys / m_tex->theight();
    int u, v;
    float x0, y0, x1, y1, u0, v0, u1, v1;
    glEnable(GL_TEXTURE_2D);
    m_tex->bind();

    glBegin(GL_QUADS);
    for (const char *p = text; ; ++p) {
        int c = (unsigned char) *p;
        switch (c) {
        case '\0':
            goto done;

        case '\n':
            xp = x;
            yp -= ys;
            break;

        default:
            u = c & 15; v = c >> 4;
            x0 = xp; x1 = xp + xs;
            y0 = yp; y1 = yp + ys;
            u0 = u * xt; u1 = u0 + xt;
            v1 = v * yt; v0 = v1 + yt;
            glTexCoord2f(u0, v0); glVertex2f(x0, y0);
            glTexCoord2f(u0, v1); glVertex2f(x0, y1);
            glTexCoord2f(u1, v1); glVertex2f(x1, y1);
            glTexCoord2f(u1, v0); glVertex2f(x1, y0);
            xp += xs;
            break;
        }
    }
done:
    glEnd();

    glDisable(GL_TEXTURE_2D);
}
