#ifndef UI_WIDGET_HPP
#define UI_WIDGET_HPP
namespace UI {
struct MouseEvent;

class Widget {
public:
    virtual ~Widget();

    virtual void draw();

    virtual void mouseMoved(MouseEvent const &evt);
    virtual void mouseEntered(MouseEvent const &evt);
    virtual void mouseExited(MouseEvent const &evt);

    virtual void mouseDown(MouseEvent const &evt);
    virtual void mouseUp(MouseEvent const &evt);
    virtual void mouseDragged(MouseEvent const &evt);
};

}
#endif
