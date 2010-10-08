#ifndef UI_LAYER_HPP
#define UI_LAYER_HPP
namespace UI {
struct Event;

class Screen {
public:
    static Screen *active;

    virtual ~Screen();
    virtual void handleEvent(Event const &evt) = 0;
    virtual void draw() = 0;
};

}
#endif
