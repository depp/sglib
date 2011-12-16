#include "error_win.hpp"
#include <string.h>
#include <Windows.h>

error_win::~error_win() throw() { }

char const *error_win::what() const throw()
{
    try {
        if (what_.empty()) {
            LPVOID text;
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                code_,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &text,
                0, NULL);
            if (text) {
                try {
                    what_ = (char *) text;
                } catch (std::exception &) { }
                LocalFree(text);
            }
        }
    } catch(std::exception &) { }
    return what_.c_str();
}
