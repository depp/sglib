#ifndef UI_WIDGET_HPP
#define UI_WIDGET_HPP
namespace UI {

class Widget {
public:
    virtual ~Widget();
    virtual void draw() = 0;
};

}
#endif
