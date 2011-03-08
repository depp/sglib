#include "stringarray.hpp"

void parseStringArray(std::vector<std::string> &val, char const *str)
{
    char const *p, *s = str;
    while (1) {
        for (p = s; *p && *p != ':'; ++p);
        if (s != p)
            val.push_back(std::string(s, p));
        if (*p == ':')
            s = p + 1;
        else
            break;
    }
}
