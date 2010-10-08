#include "container.hpp"
#include "event.hpp"

UI::Container::~Container()
{ }

void UI::Container::draw()
{
    std::vector<Widget *>::iterator
        i = children_.begin(), e = children_.end();
    for (; i != e; ++i)
        (*i)->draw();
}

void UI::Container::mouseMoved(UI::MouseEvent const &evt)
{
    if (button_ >= 0) {
        if (activeChild_)
            activeChild_->mouseMoved(evt);
    } else {
        Widget *w1 = activeChild_, *w2 = childAtPoint(evt.x, evt.y);
        activeChild_ = w2;
        if (w1)
            w1->mouseMoved(evt);
        if (w2 && w2 != w1)
            w2->mouseMoved(evt);
    }
    Widget::mouseMoved(evt);
}

void UI::Container::mouseDown(UI::MouseEvent const &evt)
{
    if (button_ >= 0) {
        if (activeChild_)
            activeChild_->mouseDown(evt);
    } else {
        Widget *w = childAtPoint(evt.x, evt.y);
        button_ = evt.button;
        activeChild_ = w;
        if (w)
            w->mouseDown(evt);
    }
}

void UI::Container::mouseUp(UI::MouseEvent const &evt)
{
    if (button_ >= 0) {
        if (activeChild_)
            activeChild_->mouseUp(evt);
        if (button_ == evt.button)
            button_ = -1;
    }
}

void UI::Container::addChild(UI::Widget *child)
{
    children_.push_back(child);
}

UI::Widget *UI::Container::childAtPoint(int x, int y)
{
    std::vector<Widget *>::iterator
        i = children_.begin(), e = children_.end();
    for (; i != e; ++i) {
        Widget *w = *i;
        if (w->bounds().contains(x, y))
            return w;
    }
    return 0;
}
