#include "item.hpp"
#include "tileset.hpp"
using namespace LD22;

Item::~Item()
{ }

void Item::draw(int delta, Tileset &tiles)
{
    int x, y;
    getDrawPos(&x, &y, delta);
    tiles.drawWidget(x, y, Widget::Star);
}

void Item::advance()
{
    Mover::advance();
}
