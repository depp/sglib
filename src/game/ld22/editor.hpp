#ifndef GAME_LD22_EDITOR_HPP
#define GAME_LD22_EDITOR_HPP
#include "screenbase.hpp"
namespace UI {
struct KeyEvent;
struct MouseEvent;
}
namespace LD22 {

class Editor : public ScreenBase {
    int m_tile;
    int m_mx, m_my;
    int m_mouse;

protected:
    virtual void drawExtra();

public:
    Editor();
    virtual ~Editor();

    virtual void handleEvent(const UI::Event &evt);

private:
    void handleKeyDown(const UI::KeyEvent &evt);
    void handleMouseDown(const UI::MouseEvent &evt);
    void handleMouseUp(const UI::MouseEvent &evt);
    void handleMouseMove(const UI::MouseEvent &evt);

    bool translateMouse(const UI::MouseEvent &evt, int *x, int *y);
};

}
#endif
