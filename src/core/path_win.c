/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "file_impl.h"
#include <Windows.h>

void
sg_path_copy(wchar_t *dest, const char *src, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i)
        dest[i] = src[i] == '/' ? SG_PATH_DIRSEP : src[i];
}

int
sg_path_mkpardir(const wchar_t *path, struct sg_error **err)
{
    wchar_t *buf;
    size_t len, i;
    int ecode;

    i = wcslen(path);
    while (i > 0 && path[i-1] != '\\' && path[i-1] != '/')
        i--;
    if (i == 0)
        return 0;
    len = i;
    buf = malloc(sizeof(wchar_t) * (len + 1));
    if (!buf) {
        sg_error_nomem(err);
        return -1;
    }
    memcpy(buf, path, sizeof(wchar_t) * len);
    buf[len] = '\0';
    while (1) {
        if (CreateDirectory(buf, NULL))
            break;
        ecode = GetLastError();
        if (ecode != ERROR_PATH_NOT_FOUND) {
            if (ecode == ERROR_ALREADY_EXISTS)
                break;
            sg_error_win32(err, ecode);
            free(buf);
            return -1;
        }
        i--;
        while (i > 0 && path[i-1] != '\\' && path[i-1] != '/')
            i--;
        while (i > 0 && (path[i-1] == '\\' || path[i-1] == '/'))
            i--;
        if (i == 0)
            return 0;
        buf[i] = L'\0';
    }
    while (i < len) {
        buf[i] = path[i];
        while (i < len && buf[i] != L'0')
            i++;
        if (!CreateDirectory(buf, NULL)) {
            ecode = GetLastError();
            if (ecode != ERROR_ALREADY_EXISTS) {
                sg_error_win32(err, ecode);
                free(buf);
                return -1;
            }
        }
    }
    free(buf);
    return 0;
}

static size_t
sg_path_getexerelpath(wchar_t *buf, size_t buflen, const char *relpath)
{
    size_t len;
    const char *p;
    len = GetModuleFileName(NULL, buf, buflen);
    if (!len || len > buflen)
        return 0;
    while (len > 0 && buf[len - 1] != L'\\')
        len--;
    if (len == 0)
        return 0;
    p = relpath;
    while (len < buflen && *p)
        buf[len++] = *p++;
    if (*p)
        return 0;
    return len;
}

size_t
sg_path_getuserpath(wchar_t *buf, size_t buflen)
{
    return sg_path_getexerelpath(buf, buflen, SG_PATH_USER);
}

size_t
sg_path_getdatapath(wchar_t *buf, size_t buflen)
{
    return sg_path_getexerelpath(buf, buflen, SG_PATH_DATA);
}
