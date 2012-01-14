#ifndef CLIENT_UI_BUTTON_HPP
#define CLIENT_UI_BUTTON_HPP
#include "widget.hpp"
#include "action.hpp"
#include "client/type.hpp"
#include <string>
namespace UI {

class Button : public Widget {
public:
    Button();
    virtual ~Button();

    void setLoc(int x, int y);
    void setText(std::string const &text);

    virtual void draw();

    // virtual void mouseMoved(MouseEvent const &evt);
    virtual void mouseEntered(MouseEvent const &evt);
    virtual void mouseExited(MouseEvent const &evt);
    virtual void mouseDown(MouseEvent const &evt);
    virtual void mouseUp(MouseEvent const &evt);

    void setAction(Action const &action);

private:
    // RasterText::Ref title_;
    TextLayout title_;
    bool state_, hover_;
    int button_;
    Action action_;
};

}
#endif
