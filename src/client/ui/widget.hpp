#ifndef UI_WIDGET_HPP
#define UI_WIDGET_HPP
#include "geometry.hpp"
#include "client/scene/leafobject.hpp"
namespace UI {
struct MouseEvent;
struct KeyEvent;
struct Event;
class Screen;

class Widget : public Scene::LeafObject {
public:
    Widget();
    virtual ~Widget();

    virtual bool hitTest(Point pt);

    virtual void mouseMoved(MouseEvent const &evt);
    virtual void mouseEntered(MouseEvent const &evt);
    virtual void mouseExited(MouseEvent const &evt);
    virtual void mouseDown(MouseEvent const &evt);
    virtual void mouseUp(MouseEvent const &evt);

    Rect const &bounds() const { return bounds_; }

protected:
    Rect bounds_;
};

}
#endif
