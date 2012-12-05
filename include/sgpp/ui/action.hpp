/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SGPP_UI_ACTION_HPP
#define SGPP_UI_ACTION_HPP
#include "screen.hpp"
namespace UI {
class Screen;
class Widget;

class Action {
public:
    typedef Screen Object;
    typedef void (Object::*Method)();

    Action()
        : object_(0), method_(0)
    { }

    Action(Object *obj, Method method)
        : object_(obj), method_(method)
    { }

    void operator()()
    {
        if (object_)
            (object_->*method_)();
    }

private:
    Object *object_;
    Method method_;
};

}
#endif
