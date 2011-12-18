#ifndef GAME_LD22_SCREEN_HPP
#define GAME_LD22_SCREEN_HPP
#include "screenbase.hpp"
#include "client/ui/keymanager.hpp"
namespace LD22 {
class Area;

class Screen : public ScreenBase {
    UI::KeyManager m_key;
    std::auto_ptr<Area> m_area;

protected:
    virtual void drawExtra(int delta);
    virtual void advance();

public:
    Screen();
    virtual ~Screen();

    virtual void handleEvent(const UI::Event &evt);
    virtual void init();

    bool getKey(int k)
    {
        return m_key.inputState(k);
    }

private:
    void startGame();
};

}
#endif
