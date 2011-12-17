#ifndef GAME_LD22_SCREEN_HPP
#define GAME_LD22_SCREEN_HPP
#include "client/ui/screen.hpp"
#include "client/ui/keymanager.hpp"
namespace LD22 {
class Area;
class Background;

class Screen : public UI::Screen {
    UI::KeyManager m_key;
    Area *m_area;
    unsigned m_tickref;
    int m_delta;
    Background *m_background;

public:
    Screen();
    virtual ~Screen();

    virtual void handleEvent(const UI::Event &evt);
    virtual void update(unsigned int ticks);
    virtual void draw();

    bool getKey(int k)
    {
        return m_key.inputState(k);
    }

private:
    void advance();
};

}
#endif
