#include "keymanager.hpp"
#include "event.hpp"
#include <cstring>

UI::KeyManager::KeyManager(const unsigned char *map)
{
    std::memset(map_, 255, sizeof(map_));
    for (int i = 0; ; ++i) {
        if (map[i*2] == 255)
            break;
        map_[map[i*2]] = map[i*2+1];
    }
    reset();
}

void UI::KeyManager::reset()
{
    std::memset(key_, 0, sizeof(key_));
    std::memset(vinput_, 0, sizeof(vinput_));
}

void UI::KeyManager::handleKeyEvent(KeyEvent const &evt)
{
    if (evt.key > 255 || evt.key < 0)
        return;
    unsigned tgt = map_[evt.key];
    if (tgt >= (unsigned) NUM_INPUT)
        return;
    unsigned idx = evt.key >> 5, mask = 1U << (evt.key & 31);
    if (evt.type == KeyDown) {
        if (!(key_[idx] & mask)) {
            key_[idx] |= mask;
            vinput_[tgt] += 1;
        }
    } else {
        if (key_[idx] & mask) {
            key_[idx] &= ~mask;
            vinput_[tgt] -= 1;
        }
    }
}
