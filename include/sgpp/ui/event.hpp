/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SGPP_UI_EVENT_HPP
#define SGPP_UI_EVENT_HPP
namespace UI {

enum EventType {
    MouseDown,
    MouseUp,
    MouseMove,
    KeyDown,
    KeyUp
};

enum {
    ButtonLeft,
    ButtonRight,
    ButtonMiddle,
    ButtonOther
};

struct MouseEvent;
struct KeyEvent;

struct Event {
    Event()
    { }

    Event(EventType type_)
        : type(type_)
    { }

    EventType type;

    MouseEvent &mouseEvent();
    MouseEvent const &mouseEvent() const;
    KeyEvent &keyEvent();
    KeyEvent const &keyEvent() const;
};

struct KeyEvent : Event {
    KeyEvent()
    { }

    KeyEvent(EventType type_, int key_)
        : Event(type_), key(key_)
    { }

    int key;
};

struct MouseEvent : Event {
    MouseEvent()
    { }

    MouseEvent(EventType type_, int button_, int x_, int y_)
        : Event(type_), button(button_), x(x_), y(y_)
    { }

    int button;
    int x, y;
};

inline MouseEvent &Event::mouseEvent()
{ return static_cast<MouseEvent &>(*this); }

inline MouseEvent const &Event::mouseEvent() const
{ return static_cast<MouseEvent const &>(*this); }

inline KeyEvent &Event::keyEvent()
{ return static_cast<KeyEvent &>(*this); }

inline KeyEvent const &Event::keyEvent() const
{ return static_cast<KeyEvent const &>(*this); }

}
#endif
