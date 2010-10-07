#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP
#include "widget.hpp"
#include "type/rastertext.hpp"
#include <string>
namespace UI {

class Button : public Widget {
public:
    Button();
    virtual ~Button();

    void setLoc(float x, float y);
    void setText(std::string const &text);

    virtual void draw();

private:
    RasterText title_;
    float x_, y_;
};

}
#endif
