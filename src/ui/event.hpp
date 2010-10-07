#ifndef UI_EVENT_HPP
#define UI_EVENT_HPP
namespace UI {

enum EventType {
    MouseDown,
    MouseUp,
    MouseMove,
    KeyDown,
    KeyUp
};

struct KeyEvent {
    EventType type;
    int key;
};

struct MouseEvent {
    EventType type;
    int button;
    int x, y;
};

union Event {
    EventType type;
    KeyEvent key;
    MouseEvent mouse;
};

}
#endif
