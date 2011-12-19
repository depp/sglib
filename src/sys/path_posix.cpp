#include "path.hpp"
#include "path_posix.hpp"
#include "ifilereg.hpp"
#include "system_error.hpp"
#include <errno.h>
#include <sys/stat.h>

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
    gDirs[0].path = ptr;
    gDirs[0].len = l;
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
            gDirs[0].path = ptr;
            gDirs[0].len = p;
        }
    }
}

#else

#error "Can't find executable on this platform"

#endif


void pathInit(const char *altpath)
{
    getExeDir();
    if (gDirs[0].path) {
        unsigned l = gDirs[0].len;
        char *b = (char *) malloc(l + 5);
        assert(b != NULL);
        memcpy(b, gDirs[0].path, l);
        memcpy(b + l, "/data", 5);
        free(const_cast<char *> (gDirs[0].path));
        gDirs[0].path = b;
        gDirs[0].len = l + 5;
    }
    if (altpath) {
        gDirs[1].path = altpath;
        gDirs[1].len = strlen(altpath);
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
    throw file_not_found();
}

FILE *Path::openOFile(std::string const &path)
{
    if (!gDirs[0].path)
        abort();
    char buf[512];
    if (gDirs[0].len > 510 || path.size() > 510 - gDirs[0].len) {
        fputs("File name too long\n", stderr);
        abort();
    }
    int i = 0, r;
    memcpy(buf, gDirs[0].path, gDirs[0].len);
    i += gDirs[0].len;
    buf[i++] = '/';
    std::string::const_iterator p = path.begin(), e = path.end();
    for (; p != e; ++p) {
        if (*p == '/') {
            buf[i] = '\0';
            r = mkdir(buf, 0777);
            if (r && errno != EEXIST) {
                perror("mkdir");
                abort();
            }
        }
        buf[i++] = *p;
    }
    buf[i] = '\0';
    FILE *f = fopen(buf, "w");
    if (!f) {
        perror("fopen");
        abort();
    }
    return f;
}
