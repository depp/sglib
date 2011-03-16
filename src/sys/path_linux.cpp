#include "path.hpp"
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
namespace Path {

std::string exeDir()
{
    char buf[PATH_MAX + 1];
    ssize_t sz = readlink("/proc/self/exe", buf, sizeof(buf));
    if (sz > 0) {
        unsigned int p = sz;
        while (p && buf[p - 1] != '/')
            --p;
        if (p) {
            if (p > 1)
                --p;
            return std::string(buf, p);
        }
    }
    return std::string();
}

}
