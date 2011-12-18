#ifndef GAME_LD22_EDITOR_HPP
#define GAME_LD22_EDITOR_HPP
#include "screenbase.hpp"
namespace UI {
struct KeyEvent;
struct MouseEvent;
}
namespace LD22 {

class Editor : public ScreenBase {
    typedef enum {
        MBrush,
        MEntity
    } Mode;

    Mode m_mode;
    int m_tile;
    int m_mx, m_my;
    int m_mouse;
    int m_ent, m_etype;

protected:
    virtual void drawExtra(int delta);

public:
    Editor();
    virtual ~Editor();

    virtual void handleEvent(const UI::Event &evt);
    virtual void init();

private:
    void handleKeyDown(const UI::KeyEvent &evt);
    void handleMouseDown(const UI::MouseEvent &evt);
    void handleMouseUp(const UI::MouseEvent &evt);
    void handleMouseMove(const UI::MouseEvent &evt);

    bool translateMouse(const UI::MouseEvent &evt, int *x, int *y);

    void setMode(Mode m);
    void incType(int delta, bool max);
    void tileBrush(int x, int y);
    void selectEntity(int x, int y, bool click);
    void newEntity(int x, int y);
    void deleteEntity();

    void open(int num);
    void save();
};

}
#endif
