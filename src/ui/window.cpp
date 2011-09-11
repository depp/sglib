#include "window.hpp"
#include "sys/clock.hpp"
#include "opengl.hpp"
#include "sys/resource.hpp"

UI::Window::~Window()
{
    if (screen_)
        delete screen_;
}

void UI::Window::setScreen(UI::Screen *s)
{
    assert(s);
    if (s != screen_) {
        if (screen_)
            delete screen_;
        screen_ = s;
        s->window_ = this;
    }
}

void UI::Window::setSize(unsigned w, unsigned h)
{
    width_ = w;
    height_ = h;
    glViewport(0, 0, w, h);
}

void UI::Window::draw()
{
    if (!screen_)
        return;
    unsigned time = getTime();
    screen_->update(time);
    Resource::loadAll();
    screen_->draw();
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        const GLubyte *s = gluErrorString(err);
        fprintf(stderr, "GL Error: %s\n", s);
    }
}
