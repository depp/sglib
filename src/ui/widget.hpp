#ifndef UI_WIDGET_HPP
#define UI_WIDGET_HPP
#include "rect.hpp"
#include "object.hpp"
namespace UI {
struct MouseEvent;
struct Event;

class Widget : public Object {
public:
    Widget();
    virtual ~Widget();

    virtual void handleEvent(Event const &evt);

    virtual void mouseMoved(MouseEvent const &evt);
    virtual void mouseEntered(MouseEvent const &evt);
    virtual void mouseExited(MouseEvent const &evt);
    virtual void mouseDown(MouseEvent const &evt);
    virtual void mouseUp(MouseEvent const &evt);

    Rect const &bounds() const { return bounds_; }

protected:
    Rect bounds_;

private:
    bool mouseWithinBounds_;
};

}
#endif
