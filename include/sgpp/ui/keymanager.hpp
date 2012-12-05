/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SGPP_UI_KEYMANAGER_HPP
#define SGPP_UI_KEYMANAGER_HPP
namespace UI {
struct KeyEvent;

/* A KeyManager associates each key code with a virtual input.  From
   key events, it tracks the state of each key code and virtual input.
   Multiple key codes may map to the same virtual input, the state of
   the virtual input is active if any of the keys mapped to it are
   active.  */
class KeyManager {
    /* Hard coded maximum number of inputs.  */
    static const int NUM_INPUT = 32;
public:
    /* The map is a list of associations KEY1, VIRT1, KEY2, VIRT2, etc
       terminated by 255.  */
    KeyManager(const unsigned char *map);

    void reset();

    void handleKeyEvent(KeyEvent const &evt);

    bool inputState(int i) const
    {
        return vinput_[i] != 0;
    }

private:
    unsigned map_[256];
    unsigned key_[8];
    unsigned char vinput_[NUM_INPUT];
};

}
#endif
