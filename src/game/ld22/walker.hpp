#ifndef GAME_LD22_WALKER_HPP
#define GAME_LD22_WALKER_HPP
#include "mover.hpp"
namespace LD22 {
class Item;

// An stick figure actor that walks around, is affected by gravity.
class Walker : public Mover {
public:
    // All push should be in -PUSH_SCALE .. +PUSH_SCALE
    static const int PUSH_SCALE = 256;
    static const int PICKUP_DISTANCE = 32;

private:
    typedef enum {
        WFall,
        WStand
    } WState;

    typedef enum {
        AStand,
        // AWaveLeft,
        // AWaveRight,
        AFall,
        ALandLeft,
        ALandRight,
        AWalkLeft,
        AWalkRight
    } WAnim;

    WState m_wstate;
    WAnim m_anim;
    int m_animtime;
    bool m_animlock;
    int m_sprite;

protected:
    int m_wclass;
    // "Push": walking and jumping
    // subclass is responsible for updating
    // These are desired speeds as a fraction of maximum
    int m_xpush, m_ypush;

public:
    explicit Walker(int wclass)
        : Mover(AWalker), m_wclass(wclass), m_xpush(0), m_ypush(0),
          m_item(0)
    {
        m_w = STICK_WIDTH;
        m_h = STICK_HEIGHT;
    }

    virtual ~Walker();
    virtual void draw(int delta, Tileset &tiles);
    virtual void init();
    virtual void advance();

    // Scan for items.  The closest item will be placed in m_item with
    // its distance.
    void scanItems();

    // Update the item information without rescanning for close items.
    // Either scanItems or updateItem should be called every update.
    void updateItem();

    // Pick up m_item.
    bool pickupItem();

    // Return whether we have a valid grab
    bool haveGrab();

    // Get the position where a grabbed item should be held
    void getGrabPos(int *x, int *y);

    Item *m_item;
    float m_item_distance;

private:
    // Check whether falling or standing
    void checkState();
};

}
#endif
