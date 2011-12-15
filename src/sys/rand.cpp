#include "rand.hpp"
#include <ctime>

Rand Rand::global = Rand();

void Rand::seed()
{
    std::time_t t;
    std::time(&t);
    x0 = t;
    x1 = 0x038acaf3U;
    c = 0xa2cc5886U;
}
