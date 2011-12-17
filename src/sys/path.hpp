#ifndef SYS_PATH_HPP
#define SYS_PATH_HPP
#include <string>
class IFile;
namespace Path {

IFile *openIFile(std::string const &path);

}
#endif
