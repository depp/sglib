/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
/* Windows file / path code.  */
#include "file_impl.h"
#include "sg/error.h"
#include "sg/file.h"

#include <Windows.h>
#include <stdio.h>

static uint64_t
sg_32to64(DWORD hi, DWORD lo)
{
    return ((uint64_t) hi << 32) | lo;
}

int
sg_reader_open(
    struct sg_reader *fp,
    const wchar_t *path,
    struct sg_error **err)
{
    HANDLE h;
    DWORD ecode;
    h = CreateFile(
        path,
        FILE_READ_DATA,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ecode = GetLastError();
        if (ecode == ERROR_FILE_NOT_FOUND || ecode == ERROR_PATH_NOT_FOUND)
            return SG_FILE_NOTFOUND;
        sg_error_win32(err, ecode);
        return SG_FILE_ERROR;
    }
    fp->handle = h;
    return SG_FILE_OK;
}

int
sg_reader_getinfo(
    struct sg_reader *fp,
    int64_t *length,
    struct sg_fileid *fileid,
    struct sg_error **err)
{
    BY_HANDLE_FILE_INFORMATION ifo;
    if (!GetFileInformationByHandle(fp->handle, &ifo)) {
        sg_error_win32(err, GetLastError());
        return -1;
    }
    *length = sg_32to64(
        ifo.nFileSizeHigh,
        ifo.nFileSizeLow);
    fileid->f_[0] = sg_32to64(
        ifo.ftLastWriteTime.dwHighDateTime,
        ifo.ftLastWriteTime.dwLowDateTime);
    fileid->f_[1] = sg_32to64(
        ifo.nFileIndexHigh,
        ifo.nFileIndexLow);
    fileid->f_[2] = ifo.dwVolumeSerialNumber;
    return 0;
}

int
sg_reader_read(
    struct sg_reader *fp,
    void *buf,
    size_t bufsz,
    struct sg_error **err)
{
    DWORD ramt, damt;
    damt = bufsz > INT_MAX ? (DWORD) INT_MAX : (DWORD) bufsz;
    if (!ReadFile(fp->handle, buf, damt, &ramt, NULL)) {
        sg_error_win32(err, GetLastError());
        return -1;
    }
    return ramt;
}

void
sg_reader_close(
    struct sg_reader *fp)
{
    CloseHandle(fp->handle);
}

struct sg_writer {
    HANDLE handle;
    wchar_t *destpath;
    wchar_t *temppath;
};

struct sg_writer *
sg_writer_open(
    const char *path,
    size_t pathlen,
    struct sg_error **err)
{
    struct sg_writer *fp;
    const wchar_t *basepath;
    size_t baselen;
    char buf[SG_MAX_PATH];
    wchar_t *destpath, *temppath;
    int nlen, r;
    HANDLE h;
    DWORD ecode;

    nlen = sg_path_norm(buf, path, pathlen, err);
    if (nlen < 0)
        return NULL;

    basepath = sg_paths.path[0].path;
    baselen = sg_paths.path[0].len;
    fp = malloc(sizeof(*fp) + sizeof(wchar_t) * ((baselen + nlen) * 2 + 6));
    if (!fp)
        goto error_nomem;
    fp->destpath = destpath = (wchar_t *) (fp + 1);
    fp->temppath = temppath = destpath + baselen + nlen + 1;
    wmemcpy(destpath, basepath, baselen);
    sg_path_copy(destpath + baselen, buf, nlen + 1);
    wmemcpy(temppath, basepath, baselen);
    sg_path_copy(temppath + baselen, buf, nlen);
    wmemcpy(temppath + baselen + nlen, L".tmp", 5);

    h = CreateFile(
        temppath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ecode = GetLastError();
        if (ecode != ERROR_PATH_NOT_FOUND)
            goto error_win32;
        r = sg_path_mkpardir(temppath, err);
        if (r != SG_FILE_OK) {
            if (r == SG_FILE_ERROR)
                goto error;
            goto error_win32;
        }
        h = CreateFile(
            temppath,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (h == INVALID_HANDLE_VALUE) {
            ecode = GetLastError();
            goto error_win32;
        }
    }
    fp->handle = h;
    return fp;

error_nomem:
    sg_error_nomem(err);
    return NULL;

error_win32:
    sg_error_win32(err, ecode);
    goto error;

error:
    free(fp);
    return NULL;
}

int64_t
sg_writer_seek(
    struct sg_writer *fp,
    int64_t offset,
    int whence,
    struct sg_error **err)
{
    DWORD lo, hi, method, ecode;
    HANDLE h = fp->handle;
    if (h == INVALID_HANDLE_VALUE) {
        sg_error_invalid(err, __FUNCTION__, "fp");
        return -1;
    }
    switch (whence) {
    case SEEK_SET: method = FILE_BEGIN; break;
    case SEEK_CUR: method = FILE_CURRENT; break;
    case SEEK_END: method = FILE_END; break;
    default:
        sg_error_invalid(err, __FUNCTION__, "whence");
        return -1;
    }
    lo = offset & 0xffffffffu;
    hi = offset >> 32;
    lo = SetFilePointer(h, lo, &hi, method);
    if (lo == INVALID_SET_FILE_POINTER) {
        ecode = GetLastError();
        sg_error_win32(err, ecode);
        return -1;
    }
    return sg_32to64(hi, lo);
}

int
sg_writer_write(
    struct sg_writer *fp,
    const void *buf,
    size_t amt,
    struct sg_error **err)
{
    DWORD ramt, damt, ecode;
    HANDLE h = fp->handle;
    if (h == INVALID_HANDLE_VALUE) {
        sg_error_invalid(err, __FUNCTION__, "fp");
        return -1;
    }
    damt = amt > INT_MAX ? (DWORD)INT_MAX : (DWORD) amt;
    if (!WriteFile(h, buf, damt, &ramt, NULL)) {
        ecode = GetLastError();
        sg_error_win32(err, ecode);
        return -1;
    }
    return ramt;
}

int
sg_writer_commit(
    struct sg_writer *fp,
    struct sg_error **err)
{
    HANDLE h;
    DWORD ecode;
    BOOL r;
    h = fp->handle;
    if (h == INVALID_HANDLE_VALUE) {
        sg_error_invalid(err, __FUNCTION__, "fp");
        return -1;
    }
    fp->handle = INVALID_HANDLE_VALUE;
    if (!FlushFileBuffers(h)) {
        CloseHandle(h);
        goto error_win32;
    }
    CloseHandle(h);
    r = ReplaceFile(
        fp->destpath,
        fp->temppath,
        NULL,
        0,
        NULL,
        NULL);
    if (!r)
        goto error_win32;
    return 0;

error_win32:
    ecode = GetLastError();
    sg_error_win32(err, ecode);
    DeleteFile(fp->temppath);
    return -1;
}

void
sg_writer_close(
    struct sg_writer *fp)
{
    HANDLE h = fp->handle;
    if (h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        DeleteFile(fp->temppath);
    }
    free(fp);
}
