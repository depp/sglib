// SDL contaminates the build system with a _GNU_SOURCE def,
// which causes a non-portable strerror_r to be declared.
#undef _GNU_SOURCE
#include "system_error.hpp"
#include <string.h>

system_error::~system_error() throw() { }

char const *system_error::what() const throw()
{
    try {
        if (what_.empty()) {
            char buf[256];
            int r = strerror_r(errno_, buf, sizeof(buf));
            if (!r)
                what_ = buf;
        }
    } catch (std::exception &e) { }
    return what_.c_str();
}
