/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
/* Windows file / path code.  */
#include "error.h"
#include "file_impl.h"
#include "file.h"

#include <Windows.h>
#include <stdio.h>

void
sg_buffer_free_(struct sg_buffer *fbuf)
{
    if (fbuf->data)
        free(fbuf->data);
    free(fbuf);
}

struct sg_file_w {
    struct sg_file h;
    HANDLE hdl;
};

static int
sg_file_w_read(struct sg_file *f, void *buf, size_t amt)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    DWORD ramt, damt;
    damt = amt > INT_MAX ? (DWORD) INT_MAX : (DWORD) amt;
    if (ReadFile(w->hdl, buf, damt, &ramt, NULL)) {
        return ramt;
    } else {
        sg_error_win32(&w->h.err, GetLastError());
        return -1;
    }
}

static int
sg_file_w_write(struct sg_file *f, const void *buf, size_t amt)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    DWORD ramt, damt;
    damt = amt > INT_MAX ? (DWORD) INT_MAX : (DWORD) amt;
    if (WriteFile(w->hdl, buf, damt, &ramt, NULL)) {
        return ramt;
    } else {
        sg_error_win32(&w->h.err, GetLastError());
        return -1;
    }
}

static int
sg_file_w_close(struct sg_file *f)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    if (w->hdl != INVALID_HANDLE_VALUE) {
        CloseHandle(w->hdl);
        w->hdl = INVALID_HANDLE_VALUE;
    }
    return 0;
}

static void
sg_file_w_free(struct sg_file *f)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    if (w->hdl != INVALID_HANDLE_VALUE)
        CloseHandle(w->hdl);
    free(w);
}

static int64_t
sg_file_w_length(struct sg_file *f)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    BY_HANDLE_FILE_INFORMATION ifo;
    if (GetFileInformationByHandle(w->hdl, &ifo)) {
        return ((int64_t) ifo.nFileSizeHigh << 32) | ifo.nFileSizeLow;
    } else {
        sg_error_win32(&w->h.err, GetLastError());
        return -1;
    }
}

static int64_t
sg_file_w_seek(struct sg_file *f, int64_t off, int whence)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    DWORD lo, hi, method, err;
    switch (whence) {
    case SEEK_SET: method = FILE_BEGIN; break;
    case SEEK_CUR: method = FILE_CURRENT; break;
    case SEEK_END: method = FILE_END; break;
    default: method = (DWORD) -1; break;
    }
    lo = off & 0xffffffffu;
    hi = off >> 32;
    lo = SetFilePointer(w->hdl, lo, &hi, method);
    if (lo == INVALID_SET_FILE_POINTER) {
        err = GetLastError();
        if (err) {
            sg_error_win32(&w->h.err, GetLastError());
            return -1;
        }
    }
    return ((int64_t) hi << 32) | lo;
}

int
sg_file_tryopen(struct sg_file **f, const wchar_t *path, int flags,
                struct sg_error **e)
{
    struct sg_file_w *w;
    HANDLE h;
    DWORD ecode;
    h = CreateFileW(
        path,
        FILE_READ_DATA,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (h != INVALID_HANDLE_VALUE) {
        w = malloc(sizeof(*w));
        if (!w) {
            CloseHandle(h);
            sg_error_nomem(e);
            return -1;
        }
        w->h.refcount = 1;
        w->h.read = sg_file_w_read;
        w->h.write = sg_file_w_write;
        w->h.close = sg_file_w_close;
        w->h.free = sg_file_w_free;
        w->h.length = sg_file_w_length;
        w->h.seek = sg_file_w_seek;
        w->hdl = h;
        *f = &w->h;
        return 1;
    } else {
        ecode = GetLastError();
        if (ecode == ERROR_FILE_NOT_FOUND || ecode == ERROR_PATH_NOT_FOUND)
            return 0;
        sg_error_win32(e, ecode);
        return -1;
    }
}

int
sg_path_getexepath(pchar *path, size_t len)
{
    DWORD dr;
    dr = GetModuleFileNameW(NULL, path, len);
    if (!dr || dr >= len)
        return 0;
    path[dr] = '\0';
    return 1;
}

int
sg_path_checkdir(const pchar *path)
{
    DWORD dr;
    dr = GetFileAttributesW(path);
    if (dr == INVALID_FILE_ATTRIBUTES)
        return 0;
    if (!(dr & FILE_ATTRIBUTE_DIRECTORY))
        return 0;
    return 1;
}
