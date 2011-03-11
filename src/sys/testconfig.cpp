#include "configfile.hpp"
#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    ConfigFile cfg;
    bool bv, r;
    long lv;
    std::string sv;
    std::vector<std::string> vv;

    char *dummy = reinterpret_cast<char *>(malloc(10000));
    if (!dummy) err(1, "malloc");
    memset(dummy, 'x', 9999);
    dummy[9999] = '\0';

    cfg.putKey("a", "a", "abcdef");
    cfg.putKey("a", "y", NULL);
    cfg.putKey("a", "x", "123");
    cfg.putKey("a", "x", "junk");
    cfg.putKey("a", "x", "whatever");
    cfg.putKey("a", "x", "more stuff");
    cfg.putKey("a", "x", dummy);
    cfg.putKey("a", "x", "987");
    cfg.putKey("a", "y", "234");
    cfg.putKey("b", "x", "345");
    cfg.putKey("b", "y", "456");
    cfg.putKey("a", "x", NULL);
    cfg.putKey("a", "y", "true");
    cfg.putKey("b", "z", "No");
    cfg.putKey("b", "w", "what:in:the::world");

    free(dummy);

    sv.clear();
    r = cfg.getString("a", "a", sv);
    assert(r && sv == "abcdef");

    assert(!cfg.hasKey("a", "x"));

    bv = false;
    r = cfg.getBool("a", "y", bv);
    assert(r);
    assert(bv);

    lv = 0;
    r = cfg.getLong("b", "x", lv);
    assert(r && lv == 345);

    lv = 0;
    r = cfg.getLong("b", "y", lv);
    assert(r && lv == 456);

    bv = true;
    r = cfg.getBool("b", "z", bv);
    assert(r && !bv);

    vv.clear();
    vv.push_back("existing");
    r = cfg.getStringArray("b", "w", vv);
    assert(r);
    assert(vv.size() == 5);
    assert(vv[0] == "existing");
    assert(vv[1] == "what");
    assert(vv[2] == "in");
    assert(vv[3] == "the");
    assert(vv[4] == "world");

    return 0;
}
