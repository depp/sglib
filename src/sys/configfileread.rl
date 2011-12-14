#include "sys/configfile.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <stdarg.h>

#define MAX_WARNINGS 10

#define WARN(...) \
    do { \
        configFileWarn(path, lineno, __VA_ARGS__); \
        if (++warncount > MAX_WARNINGS) goto done; \
    } while (0)

static void configFileWarn(char const *path, unsigned int lineno,
                           char const *msg, ...)
{
    va_list ap;
    fprintf(stderr, "%s:%u: ", path, lineno);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    putc('\n', stderr);
}

#define WARNC(msg, c) \
    do { \
        configFileWarnC(path, lineno, msg, *p); \
        if (++warncount > MAX_WARNINGS) goto done; \
    } while (0)

%%{
    machine configfile;
    write data;

    action putc {
        tmp[tmppos++] = *p;
    }
    action pute {
        switch (*p) {
        case 'n': tc = '\n'; break;
        case '\\': tc = '\\'; break;
        case '"': tc = '"'; break;
        default:
            tc = *p;
            WARN("Invalid escape sequence: \"\\%c\"", (int)tc);
            break;
        }
        tmp[tmppos++] = tc;
    }
    action startsec {
        secend = 0;
        keyend = 0;
        valend = 0;
        tmppos = 0;
    }
    action startvar {
        keyend = 0;
        valend = 0;
        tmppos = secend;
        quoted = false;
    }
    action setquoted {
        quoted = true;
    }
    action endsec {
        secend = tmppos;
    }
    action endkey {
        keyend = tmppos;
    }
    action endval {
        valend = tmppos;
    }
    action nextline {
        lineno += 1;
    }
    action endvarline {
        if (!secend)
            WARN("Variable appears outside config section");
        else {
            if (!quoted) {
                while (valend > keyend && tmp[valend - 1] == ' ')
                    valend--;
            }
            char *d = alloc(valend + 3),
                *sec = d, *key = d + secend + 1, *val = d + keyend + 2;
            memcpy(sec, tmp, secend);
            sec[secend] = '\0';
            memcpy(key, tmp + secend, keyend - secend);
            key[keyend - secend] = '\0';
            memcpy(val, tmp + keyend, valend - keyend);
            val[valend - keyend] = '\0';
            Var v = { sec, key, val };
            vars_.push_back(v);
            sorted_ = false;
        }
    }
    action error {
        WARN("Syntax error in configuration file");
        fhold;
        fgoto eaterr;
    }
    action resume {
        fhold;
        fgoto main;
    }

    lbreak = (('\r' '\n'?) | '\n') $(line,1) %(line,0) >nextline ;
    ws = (' ' | '\t')* ;
    comment = (';' | '#') (any - ('\n' | '\r'))* ;

    csec = (alnum | '.' | '-')+ @putc %endsec ;
    ckey = (alnum | '.' | '-')+ @putc %endkey ;
    cvunquotc = print - ('"' | '\\' | '#' | ';') ;
    cvunquot = ((cvunquotc - ' ') cvunquotc**) @putc ;
    cvescape = '\\' print %to(pute);
    cvplain = (print - ('"' | '\\')) @putc ;
    cvquotc = cvplain | cvescape ;
    cvquot = ('"' cvquotc* '"') >setquoted ;
    cval = (cvunquot | cvquot)? %endval ;

    varline = (ckey ws '=' ws cval ws) >startvar %endvarline ;
    secline = ('[' ws csec ws ']' ws) >startsec ;

    line = ws (varline | secline)? comment? ;

    main := (line lbreak)* line? $!error ;

    eaterr := (any - ('\n' | '\r'))* $!resume ;
}%%

void ConfigFile::read(char const *path)
{
    unsigned int warncount = 0;
    size_t bufsz = 1024, tmpsz = bufsz * 2;
    char *buf = 0, *tmp = 0;
    FILE *f = 0;
    f = fopen(path, "rb");
    if (!f) goto error;
    buf = reinterpret_cast<char *>(malloc(bufsz));
    if (!buf) goto error;
    tmp = reinterpret_cast<char *>(malloc(tmpsz));
    if (!tmp) goto error;
    try {
        int cs;
        size_t tmppos = 0, secend = 0, keyend = 0, valend = 0, amt;
        unsigned int lineno = 1;
        unsigned char tc;
        bool quoted = false;
        %% write init;
        do {
            amt = fread(buf, 1, sizeof(buf), f);
            char *p = buf, *pe = buf + amt, *eof = NULL;
            if (!amt) {
                if (ferror(f))
                    goto error;
                eof = pe;
            }
            if (tmppos + amt > tmpsz) {
                fprintf(stderr, "%s:%u: Line too long\n", path, lineno);
                break;
            }
            %% write exec;
        } while (amt);
    } catch (...) {
        free(tmp);
        free(buf);
        fclose(f);
        throw;
    }
    goto done;
error:
    warn("Error reading config file '%s'", path);
done:
    if (warncount == MAX_WARNINGS)
        fprintf(stderr, "%s: Too many warnings, aborting\n", path);
    free(tmp);
    free(buf);
    if (f) fclose(f);
    return;
}
