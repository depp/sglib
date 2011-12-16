#include "error_win.hpp"
#include <string.h>
#include <Windows.h>

error_win::~error_win() throw() { }

char const *error_win::what() const throw()
{
    LPWSTR wtext;
    LPSTR atext;
    int l1, l2, l3;
    if (!what_.empty())
        goto done;
    l1 = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code_,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR) &wtext,
        0, NULL);
    if (!l1)
        goto done;
    l2 = WideCharToMultiByte(CP_UTF8, 0, wtext, l1, NULL, 0, NULL, NULL);
    if (!l2) {
        LocalFree(wtext);
        goto done;
    }
    atext = (LPSTR) LocalAlloc(LMEM_FIXED, l2);
    if (!atext) {
        LocalFree(wtext);
        goto done;
    }
    l3 = WideCharToMultiByte(CP_UTF8, 0, wtext, l1, atext, l2,NULL, NULL);
    if (!l3) {
        LocalFree(wtext);
        LocalFree(atext);
        goto done;
    }
    LocalFree(wtext);
    try {
        what_ = std::string(atext, l3);
    } catch (std::exception &) { }
    LocalFree(atext);

done:
    return what_.c_str();
}
