#ifndef UI_MAINMENU_HPP
#define UI_MAINMENU_HPP
#include "type/rastertext.hpp"

class MainMenu {
public:
    MainMenu();
    ~MainMenu();

    void render();

private:
    bool initted_;
    RasterText title_;
    RasterText menu_[4];
};

#endif
