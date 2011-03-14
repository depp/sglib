#ifndef SYS_PATH_HPP
#define SYS_PATH_HPP
#include <string>
#include <vector>
class IFile;
namespace Path {

extern std::string userConfig;
extern std::vector<std::string> globalConfig;
extern std::string userData;
extern std::vector<std::string> gloabalData;

// Initialize all path variables
void init();

// Get the directory containing the executable
std::string getExeDir();

IFile *openIFile(std::string const &path);

}
#endif