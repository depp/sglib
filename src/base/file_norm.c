#include "file.h"
#include "error.h"

/* Return whether the given component is legal.  The input must be a
   lowercase string containing no illegal characters.  */
static int
sg_path_checkpart(const char *p, size_t len)
{
    if (!len)
        return 0;
    if (p[len-1] == ' ')
        return 0;
    if (len > 3 && p[3] == '.')
        len = 3;
    else if (len > 4 && p[4] == '.')
        len = 4;
    switch (*p) {
    case ' ': case '.':
        return 0;
    case 'a':
        if (len == 3 && p[1] == 'u' && p[2] == 'x')
            return 0;
        break;
    case 'c':
        if (len == 4 && p[1] == 'o' && p[2] == 'm' &&
            (p[3] >= '0' && p[3] <= '9'))
            return 0;
        if (len == 3 && p[1] == 'o' && p[2] == 'n')
            return 0;
        break;
    case 'l':
        if (len == 4 && p[1] == 'p' && p[2] == 't' &&
            (p[3] >= '0' && p[3] <= '9'))
            return 0;
        break;
    case 'n':
        if (len == 3 && p[1] == 'u' && p[2] == 'l')
            return 0;
        break;
    case 'p':
        if (len == 3 && p[1] == 'r' && p[2] == 'n')
            return 0;
        break;
    default:
        break;
    }
    return 1;
}

/* Table for normalizing characters in pathnames */
static const unsigned char SG_PATH_NORM[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    ' ','!',0,'#','$','%','&','\'','(',')',0,'+',',','-','.',0,
    '0','1','2','3','4','5','6','7','8','9',0,';',0,'=',0,0,
    '@','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
    'p','q','r','s','t','u','v','w','x','y','z','[',0,']','^','_',
    '`','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
    'p','q','r','s','t','u','v','w','x','y','z','{',0,'}','~',0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

int
sg_path_norm(char *buf, const char *path, struct sg_error **err)
{
    /* 's' points to the start of the current component: either 'buf'
       or the character after a '/' */
    char const *p = path;
    char *pp = buf, *s = buf, *e = buf + SG_MAX_PATH;
    unsigned int ci, co;
    while (*p == '/')
        p++;

next_char:
    ci = (unsigned char)*p++;
    co = SG_PATH_NORM[ci];
    if (co) {
        *pp++ = co;
        if (pp == e)
            goto inval_path;
        goto next_char;
    }
    switch (pp - s) {
    case 0:
        goto empty;
    case 1:
        if (s[0] == '.') {
            pp = s;
            goto empty;
        }
        goto nonempty;
    case 2:
        if (s[0] == '.' && s[1] == '.') {
            if (s == buf)
                goto inval_path;
            s -= 2;
            while (s != buf && s[-1] != '/')
                s--;
            pp = s;
            goto empty;
        }
        goto nonempty;
    default:
        goto nonempty;
    }

nonempty:
    if (!sg_path_checkpart(s, pp - s))
        goto inval_part;
    switch (ci) {
    case '\0':
        *pp = '\0';
        return pp - buf;
    case '/': case '\\':
        *pp++ = '/';
        if (pp == e)
            goto inval_path;
        s = pp;
        goto next_char;
    default:
        goto inval_char;
    }

empty:
    switch (ci) {
    case '\0':
        if (pp != buf)
            pp--;
        *pp = '\0';
        return pp - buf;
    case '/': case '\\':
        goto next_char;
    default:
        goto inval_char;
    }

inval_path: goto inval;
inval_part: goto inval;
inval_char: goto inval;
inval:
    sg_error_sets(err, &SG_ERROR_INVALPATH, 0, "invalid path");
    return -1;
}
