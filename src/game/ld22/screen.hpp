#include "client/ui/screen.hpp"
#include "client/ui/keymanager.hpp"
namespace LD22 {

class Screen : public UI::Screen {
public:
    Screen();
    virtual ~Screen();

    virtual void handleEvent(const UI::Event &evt);
    virtual void update(unsigned int ticks);
    virtual void draw();

private:
    UI::KeyManager key_;
};

}
