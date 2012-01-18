/* Windows file / path code.  */
#include "error.h"
#include "file_impl.h"
#include "file.h"

#include <Windows.h>

void
sg_buffer_destroy(struct sg_buffer *fbuf)
{
    if (fbuf->data)
        free(fbuf->data);
}

struct sg_file_w {
    struct sg_file h;
    HANDLE hdl;
};

static int
sg_file_w_read(struct sg_file *f, void *buf, size_t amt)
{
    struct sg_file_w *w = (struct sg_file_w *) f;
    DWORD ramt;
    if (ReadFile(w->hdl, buf, amt > INT_MAX ? INT_MAX : amt, &ramt, NULL)) {
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
    DWORD ramt;
    if (WriteFile(w->hdl, buf, amt > INT_MAX ? INT_MAX : amt, &ramt, NULL)) {
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
    h = CreateFileW(path, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
sg_path_getdir(pchar **abspath, size_t *abslen,
               const char *relpath, size_t rellen,
               int flags)
{
    /* Relative, absolute, executable, and working directory path.  */
    wchar_t *rpath = NULL, *apath = NULL, *epath = NULL, *wpath = NULL;
    size_t rlen, alen, elen, wlen;
    int r, ret;
    DWORD dr;
    BOOL br;

    /* FIXME: log errors / warnings */
    if (!rellen)
        return 0;
    if (memchr(relpath, '\0', rellen))
        return 0;

    /* Convert relative path to Unicode.  */
    r = MultiByteToWideChar(CP_UTF8, 0, relpath, rellen, NULL, 0);
    if (!r)
        goto error;
    rlen = r;
    rpath = malloc(sizeof(wchar_t) * (rlen + 2));
    if (!rpath)
        goto nomem;
    r = MultiByteToWideChar(CP_UTF8, 0, relpath, rellen, rpath, rlen);
    if (!r)
        goto error;

    /* Append backslash (if not present) and NUL.  */
    if (rpath[rlen-1] != L'\\')
        rpath[rlen++] = L'\\';
    rpath[rlen] = L'\0';

    if (flags & SG_PATH_EXEDIR) {
        /* If we want something relative to the executable directory,
           we change directory there and back afterwards.  */

        /* Get executable directory path */
        elen = MAX_PATH;
        while (1) {
            epath = malloc(sizeof(wchar_t) * elen);
            if (!epath)
                goto nomem;
            dr = GetModuleFileNameW(NULL, epath, elen);
            if (!dr)
                goto error;
            if (dr < elen) {
                elen = dr;
                break;
            }
            elen *= 2;
            free(epath);
        }
        elen = dr;
        while (elen > 0 && epath[elen - 1] != L'\\')
            elen--;
        epath[elen] = L'\0';

        /* Save current path */
        dr = GetCurrentDirectoryW(0, NULL);
        if (!dr)
            goto error;
        wpath = malloc(sizeof(wchar_t) * dr);
        if (!wpath)
            goto nomem;
        dr = GetCurrentDirectoryW(dr, wpath);
        if (!dr)
            goto error;
        wlen = dr;

        /* Change path */
        br = SetCurrentDirectoryW(epath);
        if (!br)
            goto error;
    }

    dr = GetFullPathNameW(rpath, 0, NULL, NULL);
    if (!dr)
        goto error;
    apath = malloc(sizeof(wchar_t) * dr);
    dr = GetFullPathNameW(rpath, dr, apath, NULL);
    if (!dr)
        goto error;
    alen = dr;

    if (!(flags & SG_PATH_NODISCARD)) {
        dr = GetFileAttributesW(apath);
        if (dr == INVALID_FILE_ATTRIBUTES) {
            dr = GetLastError();
            if (dr == ERROR_FILE_NOT_FOUND || dr == ERROR_PATH_NOT_FOUND)
                ret = 0;
            else
                goto errorcode;
        } else if (!(dr & FILE_ATTRIBUTE_DIRECTORY)) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = 1;
    }

    if (ret > 0) {
        *abspath = apath;
        *abslen = alen;
        apath = NULL;
    }

done:
    if (wpath)
        SetCurrentDirectoryW(wpath);
    free(rpath);
    free(apath);
    free(epath);
    free(wpath);
    return ret;

error:
    dr = GetLastError();
    goto errorcode;

errorcode:
    abort();
    goto done;
    
nomem:
    abort();
    goto done;
}
