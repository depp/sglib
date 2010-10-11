#ifndef UI_CONTAINER_HPP
#define UI_CONTAINER_HPP
#include "widget.hpp"
#include <vector>
namespace UI {
struct Event;

class Container : public Widget {
public:
    Container()
        : activeChild_(0), button_(-1)
    { }

    virtual ~Container();

    virtual void draw(unsigned int ticks);

    virtual void mouseMoved(MouseEvent const &evt);
    virtual void mouseDown(MouseEvent const &evt);
    virtual void mouseUp(MouseEvent const &evt);

    void event(Event const &evt);

    virtual void addChild(Widget *child);
    Widget *childAtPoint(int x, int y);

protected:
    std::vector<Widget *> children_;
    Widget *activeChild_;
    int button_;
};

}
#endif
