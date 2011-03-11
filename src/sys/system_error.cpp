#undef _GNU_SOURCE
#include "system_error.hpp"
#include <string.h>

system_error::~system_error() throw() { }

char const *system_error::what()
{
    if (what_.empty()) {
        char buf[256];
        int r = strerror_r(errno_, buf, sizeof(buf));
        if (!r)
            what_ = buf;
    }
    return what_.c_str();
}
