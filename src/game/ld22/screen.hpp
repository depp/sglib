#ifndef GAME_LD22_SCREEN_HPP
#define GAME_LD22_SCREEN_HPP
#include "screenbase.hpp"
#include "client/ui/keymanager.hpp"
namespace LD22 {

class Screen : public ScreenBase {
    UI::KeyManager m_key;

protected:
    virtual void drawExtra();

public:
    Screen();
    virtual ~Screen();

    virtual void handleEvent(const UI::Event &evt);
    virtual void init();

    bool getKey(int k)
    {
        return m_key.inputState(k);
    }
};

}
#endif
