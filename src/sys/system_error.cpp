#include "system_error.hpp"
#include <string.h>

system_error::~system_error() throw() { }

char const *system_error::what() const throw()
{
    try {
        if (what_.empty()) {
            char buf[256];
#if defined(__GLIBC__)
            char *r = strerror_r(code_, buf, sizeof(buf));
            if (r)
                what_ = r;
#else
            int r = strerror_r(code_, buf, sizeof(buf));
            if (!r)
                what_ = buf;
#endif
        }
    } catch (std::exception &) { }
    return what_.c_str();
}
