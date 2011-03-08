#include "configfile.hpp"
#include <algorithm>
#include <limits>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <err.h>

ConfigFile::ConfigFile()
    : vars_(), data_(0), datasz_(0), dataalloc_(0), sorted_(true)
{ }

ConfigFile::~ConfigFile()
{
    free(data_);
}

/*
elems $ listArray ('\0','\255') (repeat 0) //
    [(c, 1) | c <- ['\32'..'\126']] //
    [(c, 0) | c <- "\"\\#;"]
*/
static unsigned char const CONFIG_CHAR[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static void writeConfigStr(FILE *f, char const *str)
{
    size_t nc = 0, nuq = 0;
    unsigned char const *s
        = reinterpret_cast<unsigned char const *>(str), *p;
    if (!*s)
        return;
    for (p = s; *p; ++p) {
        nc += 1;
        nuq += CONFIG_CHAR[*p];
    }
    if (nuq == nc && s[0] != ' ' && s[nc-1] != ' ') {
        fwrite(str, 1, nc, f);
        return;
    }
    unsigned char buf[256], c;
    size_t bufp = 1;
    buf[0] = '"';
    for (p = s; (c = *p); ++p) {
        if (bufp >= sizeof(buf) - 11) {
            fwrite(buf, 1, bufp, f);
            bufp = 0;
        }
        if (c >= 0x20 && c <= 0x7e) {
            if (c == '"' || c == '\\') {
                buf[bufp + 0] = '\\';
                buf[bufp + 1] = c;
                bufp += 2;
            } else {
                buf[bufp] = c;
                bufp++;
            }
        } else {
            if (c == '\n') {
                buf[bufp + 0] = '\\';
                buf[bufp + 1] = 'n';
                bufp += 2;
            }
            // Other characters silently dropped
        }
    }
    buf[bufp] = '"';
    bufp += 1;
    fwrite(buf, 1, bufp, f);
}

void ConfigFile::write(char const *path)
{
    if (!sorted_)
        sort();
    FILE *f = fopen(path, "wb");
    if (!f) goto error;
    try {
        char const *sec = "";
        std::vector<Var>::const_iterator i = vars_.begin(), e = vars_.end();
        for (; i != e; ++i) {
            Var v = *i;
            if (strcmp(v.sec, sec)) {
                fprintf(f, "[%s]\n", v.sec);
                sec = v.sec;
            }
            fprintf(f, v.val ? "%s = " : "%s =", v.key);
            writeConfigStr(f, v.val);
            putc('\n', f);
        }
    } catch (...) {
        fclose(f);
        throw;
    }
    return;
error:
    warn("Could not write config file '%s'", path);
    return;
}

void ConfigFile::dump()
{
    std::vector<Var>::iterator i = vars_.begin(), e = vars_.end();
    for (; i != e; ++i)
        printf("%s.%s = %s\n", i->sec, i->key, i->val);
}

void ConfigFile::putKey(char const *sec, char const *key, char const *val)
{
    // FIXME sanitize sec / key
    if (val) {
        Var z = { 0, 0, 0 };
        vars_.push_back(z);
        try {
            Var &v = vars_.back();
            v.sec = putStr(sec);
            v.key = putStr(key);
            v.val = val ? putStr(val) : 0;
            sorted_ = false;
        } catch (...) {
            vars_.pop_back();
            throw;
        }
    } else {
        Var *p = getVar(sec, key);
        if (p) p->val = 0;
    }
}

char const *ConfigFile::getKey(char const *sec, char const *key)
{
    Var *p = getVar(sec, key);
    return p ? p->val : 0;
}

void ConfigFile::putLong(char const *sec, char const *key, long val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", val);
    putKey(sec, key, buf);
}

void ConfigFile::putString(char const *sec, char const *key,
                           std::string const &val)
{
    // FIXME sanitize
    putKey(sec, key, val.c_str());
}

void ConfigFile::putStringArray(char const *sec, char const *key,
                                std::vector<std::string> const &val)
{
    // FIXME sanitize
    std::vector<std::string>::const_iterator
        b = val.begin(), i, e = val.end();
    size_t sz = 0;
    for (i = b; i != e; ++i) {
        size_t ssz = i->size();
        if (!ssz)
            continue;
        if (ssz > std::numeric_limits<size_t>::max() - sz)
            throw std::bad_alloc();
        sz += ssz;
    }
    Var z = { 0, 0, 0 };
    vars_.push_back(z);
    try {
        Var &v = vars_.back();
        v.sec = putStr(sec);
        v.key = putStr(key);
        if (sz) {
            char *p = alloc(sz);
            v.val = p;
            for (i = b; i != e; ++i) {
                size_t ssz = i->size();
                if (!ssz)
                    continue;
                memcpy(p, i->data(), ssz);
                p[ssz] = ':';
                p += ssz + 1;
            }
            p[-1] = '\0';
        } else
            v.val = "";
    } catch (...) {
        vars_.pop_back();
        throw;
    }
}

void ConfigFile::putBool(char const *sec, char const *key, bool val)
{
    putKey(sec, key, val ? "true" : "false");
}

void ConfigFile::putDouble(char const *sec, char const *key, double val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%g", val);
    putKey(sec, key, buf);
}

bool ConfigFile::getLong(char const *sec, char const *key, long &val)
{
    // FIXME check for overflow?  Or do we even care?
    char const *v = getKey(sec, key);
    if (!v) return false;
    char *e;
    long r = strtol(v, &e, 0);
    if (*e) return false; // FIXME warning?
    val = r;
    return true;
}

bool ConfigFile::getString(char const *sec, char const *key,
                           std::string &val)
{
    char const *v = getKey(sec, key);
    if (!v) return false;
    val = v;
    return true;
}

bool ConfigFile::getStringArray(char const *sec, char const *key,
                                std::vector<std::string> &val)
{
    char const *v = getKey(sec, key), *s = v, *p;
    if (!v) return false;
    while (1) {
        for (p = s; *p && *p != ':'; ++p);
        if (s != p)
            val.push_back(std::string(s, p));
        if (*p == ':')
            s = p + 1;
        else
            break;
    }
    return true;
}

bool ConfigFile::getBool(char const *sec, char const *key, bool &val)
{
    char const *v = getKey(sec, key);
    if (!v) return false;
    char tmp[6];
    unsigned int i;
    for (i = 0; i < 6 && v[i]; ++i)
        tmp[i] = v[i] & ~('a' ^ 'A');
    for (; i < 6; ++i)
        tmp[i] = '\0';
    bool r;
    if (!memcmp(tmp, "TRUE", 5))
        r = true;
    else if (!memcmp(tmp, "FALSE", 6))
        r = false;
    else if (!memcmp(tmp, "YES", 4))
        r = true;
    else if (!memcmp(tmp, "NO", 3))
        r = false;
    else if (!memcmp(tmp, "ON", 3))
        r = true;
    else if (!memcmp(tmp, "OFF", 4))
        r = false;
    else
        return false; // FIXME warning?
    val = r;
    return true;
}

bool ConfigFile::getDouble(char const *sec, char const *key, double val)
{
    // FIXME check for overflow?  Or do we even care?
    char const *v = getKey(sec, key);
    if (!v) return false;
    char *e;
    double r = strtod(v, &e);
    if (*e) return false; // FIXME warning?
    val = r;
    return true;
}

struct ConfigVarCmp {
    bool operator()(ConfigFile::Var const &x, ConfigFile::Var const &y)
    {
        int v;
        v = strcmp(x.sec, y.sec);
        if (v < 0) return true;
        if (v > 0) return false;
        v = strcmp(x.key, y.key);
        return v < 0;
    }
};

ConfigFile::Var *ConfigFile::getVar(char const *sec, char const *key)
{
    if (!sorted_)
        sort();
    std::vector<Var>::iterator i;
    Var v;
    v.sec = sec;
    v.key = key;
    i = std::lower_bound(vars_.begin(), vars_.end(), v, ConfigVarCmp());
    if (i != vars_.end() && !strcmp(i->sec, sec) && !strcmp(i->key, key))
        return &*i;
    return 0;
}

char *ConfigFile::alloc(size_t len)
{
    if (len > std::numeric_limits<size_t>::max()/2 - dataalloc_)
        throw std::bad_alloc();
    if (dataalloc_ < datasz_ + len) {
        size_t nalloc = dataalloc_ ? dataalloc_ : 4*1024;
        while (nalloc < datasz_ + len)
            nalloc *= 2;
        char *ndata = reinterpret_cast<char *>(malloc(nalloc));
        if (!ndata) throw std::bad_alloc();
        size_t off = 0;
        std::vector<Var>::iterator
            i = vars_.begin(), e = vars_.end();
        for (; i != e; ++i) {
            char const *s[3] = { i->sec, i->key, i->val };
            for (unsigned int j = 0; j < 3; ++j) {
                char const *str = s[j];
                if (!str)
                    continue;
                size_t ssz = strlen(str) + 1;
                s[j] = ndata + off;
                memcpy(ndata + off, str, ssz);
                off += ssz;
            }
            i->sec = s[0];
            i->key = s[1];
            i->val = s[2];
        }
        free(data_);
        assert(off + len <= nalloc);
        datasz_ = off;
        data_ = ndata;
        dataalloc_ = nalloc;
    }
    char *dest = data_ + datasz_;
    datasz_ += len;
    return dest;
}

char *ConfigFile::putStr(char const *str)
{
    size_t len = strlen(str) + 1;
    char *dest = alloc(len);
    memcpy(dest, str, len);
    return dest;
}

static bool varEq(ConfigFile::Var &x, ConfigFile::Var &y)
{
    return !strcmp(x.sec, y.sec) && !strcmp(x.key, y.key);
}

void ConfigFile::sort()
{
    if (sorted_)
        return;
    if (vars_.empty()) {
        sorted_ = true;
        return;
    }
    std::vector<Var>::iterator b = vars_.begin(), e = vars_.end(), i, o;
    std::stable_sort(b, e, ConfigVarCmp());
    for (o = b, i = b; i != e; ) {
        do ++i;
        while (i != e && varEq(*i, *(i - 1)));
        if ((i - 1)->val) {
            *o = *(i - 1);
            ++o;
        }
    }
    vars_.resize(o - b);
    sorted_ = true;
};
