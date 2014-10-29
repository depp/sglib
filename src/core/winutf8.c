/* Copyright 2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"

#include "private.h"
#include <Windows.h>
#include <limits.h>
#include <string.h>

int
sg_wchar_from_utf8(wchar_t **dest, int *destlen,
                   const char *src, size_t srclen)
{
    wchar_t *wtext;
    int wlen, r;

    if (!srclen) {
        *dest = NULL;
        if (destlen != NULL)
            *destlen = 0;
        return 0;
    }

    if (srclen > INT_MAX)
        return -1;
    wlen = MultiByteToWideChar(CP_UTF8, 0, src, (int)srclen, NULL, 0);
    if (!wlen)
        return -1;
    wtext = malloc(sizeof(wchar_t) * wlen);
    if (!wtext)
        return -1;
    r = MultiByteToWideChar(CP_UTF8, 0, src, (int)srclen, wtext, wlen);
    if (!r) {
        free(wtext);
        return -1;
    }

    *dest = wtext;
    if (destlen != NULL)
        *destlen = wlen;
    return 0;
}

int
sg_utf8_from_wchar(
    char **dest,
    size_t *destlen,
    const wchar_t *src,
    size_t srclen)
{
    char *text;
    int len, r;

    if (srclen > INT_MAX)
        return -1;
    len = WideCharToMultiByte(
        CP_UTF8, 0, src, (int)srclen, NULL, 0, NULL, NULL);
    if (!len)
        return -1;
    text = malloc(len);
    if (!text)
        return -1;
    r = WideCharToMultiByte(
        CP_UTF8, 0, src, (int)srclen, text, len, NULL, NULL);
    if (!r) {
        free(text);
        return -1;
    }

    *dest = text;
    if (destlen != NULL)
        *destlen = len;
    return 0;
}
