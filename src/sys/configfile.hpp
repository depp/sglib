#ifndef SYS_CONFIGFILE_HPP
#define SYS_CONFIGFILE_HPP
#include <string>
#include <vector>

class ConfigFile {
public:
    struct Var {
        char const *sec;
        char const *key;
        char const *val;
    };

    ConfigFile();
    ~ConfigFile();

    void read(char const *path);
    void write(char const *path);

    void dump();

    // Generic put / get, keys & values will be copied
    // Putting a NULL value deletes the key
    void putKey(char const *sec, char const *key, char const *val);
    char const *getKey(char const *sec, char const *key);

    // Convenience functions
    bool hasKey(char const *sec, char const *key)
    { return getKey(sec, key); }
    void deleteKey(char const *sec, char const *key)
    { putKey(sec, key, 0); }

    // Typed put, overwriting existing value
    void putLong(char const *sec, char const *key, long val);
    void putString(char const *sec, char const *key,
                   std::string const &val);
    void putStringArray(char const *sec, char const *key,
                        std::vector<std::string> const &val);
    void putBool(char const *sec, char const *key, bool val);
    void putDouble(char const *sec, char const *key, double val);

    // Typed get
    bool getLong(char const *sec, char const *key, long &val);
    bool getString(char const *sec, char const *key, std::string &val);
    // Note that this will APPEND to the existing vector and it will
    // not remove existing elements.
    bool getStringArray(char const *sec, char const *key,
                        std::vector<std::string> &val);
    bool getBool(char const *sec, char const *key, bool &val);
    bool getDouble(char const *sec, char const *key, double &val);

private:
    Var *getVar(char const *sec, char const *key);
    char *alloc(size_t len);
    char *putStr(char const *str);
    void sort();

    std::vector<Var> vars_;
    char *data_;
    size_t datasz_, dataalloc_;
    bool sorted_;

    ConfigFile(ConfigFile const &);
    ConfigFile &operator=(ConfigFile const &);
};

#endif
