#ifndef SYS_RAND_HPP
#define SYS_RAND_HPP
#include <stdint.h>
#include <cmath>

class Rand {
public:
    static Rand global;

    Rand()
        : x0(0), x1(0), c(0)
    { }

    void seed();

    unsigned int irand()
    {
        uint64_t y = (uint64_t)x0 * A + c;
        x0 = x1;
        x1 = y;
        c = y >> 32;
        return y;
    }

    static unsigned int girand()
    {
        return global.irand();
    }

    float frand()
    {
        return static_cast<float>(irand()) * (1.0f / 4294967296.0f);
    }

    static float gfrand()
    {
        return global.frand();
    }

private:
    static const uint32_t A = 4284966893U;
    uint32_t x0, x1, c;
};

#endif
