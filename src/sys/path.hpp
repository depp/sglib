#ifndef SYS_PATH_HPP
#define SYS_PATH_HPP
#include <string>
#include <stdio.h>
#include <stdexcept>
class IFile;
namespace Path {

class file_not_found : public std::exception {
public:
    file_not_found() throw() { }
    virtual ~file_not_found() throw();
    virtual const char *what() const throw();
};

IFile *openIFile(std::string const &path);
FILE *openOFile(std::string const &path);

}
#endif
