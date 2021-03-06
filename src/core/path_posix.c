/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "file_impl.h"
#include "sg/error.h"
#include "sg/file.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void
sg_path_copy(char *dest, const char *src, size_t len)
{
    memcpy(dest, src, len);
}

int
sg_path_mkpardir(char *path, struct sg_error **err)
{
    char *p, *end;
    int r, ecode;

    p = strrchr(path, '/');
    if (!p || p == path)
        return 0;
    end = p;
    *p = '\0';
    while (1) {
        r = mkdir(path, 0777);
        if (!r)
            break;
        ecode = errno;
        if (ecode != ENOENT) {
            if (ecode == EEXIST)
                break;
            sg_error_errno(err, ecode);
            return SG_FILE_ERROR;
        }
        p = strrchr(path, '/');
        if (!p || p == path)
            return SG_FILE_NOTFOUND;
        *p = '\0';
    }
    while (1) {
        *p = '/';
        if (p == end)
            return SG_FILE_OK;
        p += strlen(p);
        r = mkdir(path, 0777);
        if (r) {
            ecode = errno;
            if (ecode != EEXIST) {
                sg_error_errno(err, ecode);
                return SG_FILE_ERROR;
            }
        }
    }
}

#if defined __APPLE__
#include <CoreFoundation/CFBundle.h>

static size_t
sg_path_getexerelpath(char *buf, size_t buflen, CFStringRef relpath)
{
    CFBundleRef bundle;
    CFURLRef url1, url2;
    Boolean r;

    bundle = CFBundleGetMainBundle();
    if (!bundle)
        return 0;

    url1 = CFBundleCopyBundleURL(bundle);
    if (!url1)
        return 0;

    url2 = CFURLCreateCopyDeletingLastPathComponent(NULL, url1);
    CFRelease(url1);
    if (!url2)
        return 0;

    url1 = CFURLCreateCopyAppendingPathComponent(NULL, url2, relpath, true);
    CFRelease(url2);
    if (!url2)
        return 0;

    r = CFURLGetFileSystemRepresentation(url1, false, (UInt8 *) buf, buflen);
    CFRelease(url1);
    if (!r)
        return 0;

    return strlen(buf);
}

size_t
sg_path_getuserpath(char *buf, size_t buflen)
{
    return sg_path_getexerelpath(buf, buflen, CFSTR(SG_PATH_USER));
}

size_t
sg_path_getdatapath(char *buf, size_t buflen)
{
    return sg_path_getexerelpath(buf, buflen, CFSTR(SG_PATH_DATA));
}

#if 0

size_t
sg_path_getdatapath(char *buf, size_t buflen)
{
    CFBundleRef bundle;
    CFURLRef url;
    Boolean r;
    bundle = CFBundleGetMainBundle();
    if (!bundle)
        return 0;
    url = CFBundleCopyBundleURL(bundle);
    if (!url)
        return 0;
    r = CFURLGetFileSystemRepresentation(url, false, (UInt8 *) buf, buflen);
    CFRelease(url);
    if (!r)
        return 0;
    return strlen(buf);
}

#endif

#elif defined __linux__

static size_t
sg_path_getexerelpath(char *buf, size_t buflen, const char *relpath)
{
    ssize_t r;
    size_t len, rellen;
    r = readlink("/proc/self/exe", buf, buflen);
    if (r < 0 || (size_t) r >= buflen)
        return 0;
    len = r;
    while (len > 0 && buf[len - 1] != '/')
        len--;
    if (len == 0)
        return 0;
    rellen = strlen(relpath);
    if (rellen > buflen - len)
        return 0;
    memcpy(buf + len, relpath, rellen);
    return len + rellen;
}

size_t
sg_path_getuserpath(char *buf, size_t buflen)
{
    return sg_path_getexerelpath(buf, buflen, SG_PATH_USER);
}

size_t
sg_path_getdatapath(char *buf, size_t buflen)
{
    return sg_path_getexerelpath(buf, buflen, SG_PATH_DATA);
}

#else

#error "No path implementation for this platform"

#endif
