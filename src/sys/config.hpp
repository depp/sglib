#ifndef SYS_CONFIG_HPP
#define SYS_CONFIG_HPP
#include <string>
#include <vector>
namespace Config {

void init();

bool getLong(char const *sec, char const *key, long &val);
bool getString(char const *sec, char const *key, std::string &val);
// Note that this will APPEND to the existing vector and it will
// not remove existing elements.
bool getStringArray(char const *sec, char const *key,
                    std::vector<std::string> &val);
bool getBool(char const *sec, char const *key, bool &val);
bool getDouble(char const *sec, char const *key, double val);

}
#endif
