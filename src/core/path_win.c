/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "file_impl.h"
#include "sg/error.h"
#include "sg/file.h"
#include <Windows.h>

void
sg_path_copy(wchar_t *dest, const char *src, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i)
        dest[i] = src[i] == '/' ? SG_PATH_DIRSEP : src[i];
}

/* Returns the first part of the path not counting the drive specification.
   For "C:\dir\file.txt", returns "\dir\file.txt".
   For "\\server\mount\dir\file.txt", returns "\dir\file.txt".  */
static wchar_t *
sg_path_skipdrive(wchar_t *path)
{
    wchar_t *p, *q;
    if (path[0] == '\\' && path[1] == '\\') {
        p = wcschr(path + 2, '\\');
        if (!p)
            return path;
        q = wcschr(p + 1, '\\');
        if (q == p + 1)
            return path;
        if (!q)
            return path + wcslen(path);
        return q;
    }
    if (path[0] && path[1] == ':')
        return path + 2;
    return path;
}

int
sg_path_mkpardir(wchar_t *path, struct sg_error **err)
{
    wchar_t *start, *p, *end;
    DWORD ecode;
    BOOL r;

    start = sg_path_skipdrive(path);
    p = wcsrchr(start, '\\');
    if (!p || p == start || !*p)
        return SG_FILE_NOTFOUND;
    end = p;
    *p = '\0';
    while (1) {
        r = CreateDirectory(path, NULL);
        if (r)
            break;
        ecode = GetLastError();
        if (ecode != ERROR_PATH_NOT_FOUND) {
            if (ecode == ERROR_ALREADY_EXISTS)
                break;
            sg_error_win32(err, ecode);
            return SG_FILE_ERROR;
        }
        p = wcsrchr(start, '\\');
        if (!p || p == start)
            return SG_FILE_NOTFOUND;
        *p = '\0';
    }
    while (1) {
        *p = '\\';
        if (p == end)
            return SG_FILE_OK;
        p += wcslen(p);
        r = CreateDirectory(path, NULL);
        if (!r) {
            ecode = errno;
            if (ecode != ERROR_ALREADY_EXISTS) {
                sg_error_win32(err, ecode);
                return SG_FILE_ERROR;
            }
        }
    }
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
