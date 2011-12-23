#include "path.hpp"
namespace Path {

file_not_found::~file_not_found() throw()
{ }

const char *file_not_found::what() const throw()
{
    return "file not found";
}

}
