#include "path.hpp"
#include "configfile.hpp"
#include "stringarray.hpp"
#include <stdlib.h>

#ifdef PACKAGE_NAME
#define SUBDIR ("/" PACKAGE_NAME)
#else
// #error "No PACKAGE_NAME defined"
// FIXME: Get this defined properly
#define SUBDIR "/game-fixme"
#endif

namespace Path {

/* The "dev config" is a file named "config.ini" in the same path as
   the executable.  It is used to allow a development executable to
   find resources that have not been installed yet.  The dev config is
   only used for setting paths and other variables are ignored.  */
static void loadDevConfig()
{
    std::string path = exeDir();
    if (path.empty())
        return;
    path += "/config.ini";
    ConfigFile config;
    config.read(path.c_str());
    config.getString("path", "config.user", userConfig);
    config.getString("path", "data.user", userData);
    config.getStringArray("path", "config.global", globalConfig);
    config.getStringArray("path", "data.global", globalData);
}

void init()
{
    static bool initted = false;
    if (initted)
        return;
    initted = true;

    // The dev config file has highest precedence
    loadDevConfig();

    std::vector<std::string>::iterator i, e;
    char const *env;
    std::string home;
    env = getenv("HOME");
    if (env) home = env;

    // XDG data locations

    if (userConfig.empty()) {
        // User config $XDG_CONFIG_HOME, otherwise $HOME/.config
        env = getenv("XDG_CONFIG_HOME");
        if (env)
            userConfig = env;
        if (userConfig.empty() && !home.empty())
            userConfig = home + "/.config";
        if (!userConfig.empty())
            userConfig += SUBDIR;
    }

    if (globalConfig.empty()) {
        // Global config $XDG_CONFIG_DIRS, otherwise /etc/xdg
        env = getenv("XDG_CONFIG_DIRS");
        if (env)
            parseStringArray(globalConfig, env);
        if (globalConfig.empty())
            globalConfig.push_back("/etc/xdg");
        i = globalConfig.begin();
        e = globalConfig.end();
        for (; i != e; ++i) *i += SUBDIR;
    }

    if (userData.empty()) {
        // User data $XDG_DATA_HOME, otherwise $HOME/.local/share
        env = getenv("XDG_DATA_HOME");
        if (env)
            userData = env;
        if (userData.empty() && !home.empty())
            userData = home + "/.local/share";
        if (!userData.empty())
            userData += SUBDIR;
    }

    if (globalData.empty()) {
        // Global data $XDG_DATA_DIRS,
        // otherwise /usr/local/share/:/usr/share/
        env = getenv("XDG_DATA_DIRS");
        if (env)
            parseStringArray(globalData, env);
        if (globalData.empty()) {
            globalData.push_back("/usr/local/share");
            globalData.push_back("/usr/share");
        }
        i = globalData.begin();
        e = globalData.end();
        for (; i != e; ++i) *i += SUBDIR;
    }
}

}
