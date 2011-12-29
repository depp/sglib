#ifndef CLIENT_UI_WINDOW_HPP
#define CLIENT_UI_WINDOW_HPP
/* The window is the interface between platform code and common
   code.  */
#include "screen.hpp"
namespace UI {
class Screen;
namespace Window {

extern int width;
extern int height;

void setScreen(Screen *s);
void close();

}
}
#endif
