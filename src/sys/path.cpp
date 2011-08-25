#include "path.hpp"
#include "ifilereg.hpp"
#include "configfile.hpp"
#include "stringarray.hpp"
#include "system_error.hpp"
#include <errno.h>

namespace Path {

std::string userConfig;
std::vector<std::string> globalConfig;
std::string userData;
std::vector<std::string> globalData;

IFile *openIFile(std::string const &path)
{
    std::string abspath;
    if (!userData.empty()) {
        abspath = userData + '/' + path;
        try {
            return IFileReg::open(abspath);
        } catch (system_error const &) { }
    }
    std::vector<std::string>::const_iterator
        i = globalData.begin(), e = globalData.end();
    for (; i != e; ++i) {
        if (i->empty()) continue;
        abspath = *i + '/' + path;
        try {
            return IFileReg::open(abspath);
        } catch (system_error const &) { }
    }
    throw system_error(ENOENT);
}

}
