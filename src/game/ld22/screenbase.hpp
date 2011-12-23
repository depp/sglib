#ifndef GAME_LD22_SCREENBASE_HPP
#define GAME_LD22_SCREENBASE_HPP
#include "client/ui/screen.hpp"
#include "client/letterbox.hpp"
#include "level.hpp"
#include <memory>
class BitmapFont;
namespace LD22 {
class Background;
class Tileset;

class ScreenBase : public UI::Screen {
    // Width / height of screen, in pixels
    int m_width, m_height;
    Letterbox m_letterbox;

    // Timer info
    unsigned m_tickref;
    int m_delta;

    // State
    bool m_init;

    // Level data
    Level m_level;

    // Loaded level structures
    std::auto_ptr<Background> m_background;
    std::auto_ptr<Tileset> m_tileset;
    std::auto_ptr<BitmapFont> m_font;

protected:
    void setSize(int w, int h)
    {
        m_letterbox.setISize(w, h);
        m_width = w;
        m_height = h;
    }

    // Override, subclass can draw on top of level
    // orthographic projection 0..w, 0..h from setSize
    // will be set up
    virtual void drawExtra(int delta) = 0;

    void convert(int &x, int &y)
    {
        m_letterbox.convert(x, y);
    }

    // Initialize background and tileset
    void loadLevel();

    virtual void advance();

public:
    ScreenBase();
    virtual ~ScreenBase();

    virtual void update(unsigned int ticks);
    virtual void draw();

    Level &level()
    {
        return m_level;
    }

    Tileset &tileset()
    {
        return *m_tileset.get();
    }

    BitmapFont &font()
    {
        return *m_font.get();
    }

private:
};

}
#endif
