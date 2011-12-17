#ifndef GAME_LD22_EDITOR_HPP
#define GAME_LD22_EDITOR_HPP
#include "screenbase.hpp"
namespace LD22 {

class Editor : public ScreenBase {
protected:
    virtual void drawExtra();

public:
    Editor();
    virtual ~Editor();

    virtual void handleEvent(const UI::Event &evt);
};

}
#endif
