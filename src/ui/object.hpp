#ifndef UI_OBJECT_HPP
#define UI_OBJECT_HPP
namespace UI {
struct Event;

class Object {
public:
    virtual ~Object();

    virtual void draw();
    virtual void handleEvent(Event const &evt);
};

}
#endif
