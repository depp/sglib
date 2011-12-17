#include "path.hpp"
#include "path_posix.hpp"
#include "ifilereg.hpp"
#include "system_error.hpp"
#include <errno.h>

struct path_dir {
    const char *path;
    unsigned len;
};

static path_dir gDirs[2];

#if defined(__APPLE__)
#include "sys/autocf_osx.hpp"
#include <CoreFoundation/CFBundle.h>

static void getExeDir()
{
    std::string resources, bundle;
    CFBundleRef main = CFBundleGetMainBundle();
    AutoCF<CFURLRef> bundleURL(CFBundleCopyBundleURL(main));
    char buf[1024]; // FIXME maximum?
    bool r;
    r = CFURLGetFileSystemRepresentation(
        bundleURL, true, reinterpret_cast<UInt8 *>(buf), sizeof(buf));
    if (!r)
        return;
    unsigned l = strlen(buf);
    while (l && buf[l - 1] != '/')
        l--;
    if (l <= 1)
        return;
    l--;
    char *ptr = (char *) malloc(l);
    if (!ptr)
        abort();
    memcpy(ptr, buf, l);
    gDirs[1].path = ptr;
    gDirs[1].len = l;
}

#elif defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void getExeDir()
{
    char buf[512];
    ssize_t sz = readlink("/proc/self/exe", buf, sizeof(buf));
    if (sz > 0) {
        unsigned int p = sz;
        while (p && buf[p - 1] != '/')
            --p;
        if (p) {
            if (p > 1)
                --p;
            char *ptr = (char *) malloc(p);
            if (!ptr)
                abort();
            memcpy(ptr, buf, p);
            gDirs[1].path = ptr;
            gDirs[1].len = p;
        }
    }
}

#else

#error "Can't find executable on this platform"

#endif


void pathInit(const char *altpath)
{
    getExeDir();
    if (altpath) {
        gDirs[0].path = altpath;
        gDirs[0].len = strlen(altpath);
    }
}

IFile *Path::openIFile(std::string const &path)
{
    std::string abspath;
    for (int i = 0; i < 2; ++i) {
        if (!gDirs[i].path)
            continue;
        abspath = std::string(gDirs[i].path, gDirs[i].len);
        abspath += '/';
        abspath += path;
        try {
            return IFileReg::open(abspath);
        } catch (system_error &e) {
            if (e.code() != ENOENT)
                throw;
        }
    }
    throw system_error(ENOENT);
}
