#ifndef SYS_PATH_HPP
#define SYS_PATH_HPP
#include <string>
#include <stdio.h>
class IFile;
namespace Path {

IFile *openIFile(std::string const &path);
FILE *openOFile(std::string const &path);

}
#endif
