#include "file.h"
#include "error.h"
#include <string.h>

static void
sg_path_error(struct sg_error **err, const char *msg)
{
    sg_error_sets(err, &SG_ERROR_INVALPATH, 0, msg);
}

/*
  Return 0 if the the given component is legal, -1 otherwise.  Sets an
  error if the component is not legal.  The input must be a lowercase
  string containing no illegal characters.  This catches the filenames
  which are special on Windows: AUX, COM[1-9], CON, LPT[1-9], NUL,
  and PRN, as well as those names followed by an extension.

  References:
  http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
*/
static int
sg_path_checkpart(const char *p, size_t len, struct sg_error **err)
{
    char buf[5];

    if (len > 3 && p[3] == '.')
        len = 3;
    else if (len > 4 && p[4] == '.')
        len = 4;

    switch (*p) {
    case '.':
        sg_path_error(err, "file names may not start with '.'");
        return -1;

    case 'a':
        if (len == 3 && p[1] == 'u' && p[2] == 'x')
            goto reserved;
        break;

    case 'c':
        if (len == 4 && p[1] == 'o' && p[2] == 'm' &&
            (p[3] >= '0' && p[3] <= '9'))
            goto reserved;
        if (len == 3 && p[1] == 'o' && p[2] == 'n')
            goto reserved;
        break;

    case 'l':
        if (len == 4 && p[1] == 'p' && p[2] == 't' &&
            (p[3] >= '0' && p[3] <= '9'))
            goto reserved;
        break;

    case 'n':
        if (len == 3 && p[1] == 'u' && p[2] == 'l')
            goto reserved;
        break;

    case 'p':
        if (len == 3 && p[1] == 'r' && p[2] == 'n')
            goto reserved;

    default:
        break;
    }
    return 0;

reserved:
    memcpy(buf, p, len);
    buf[len] = '\0';
    sg_error_setf(err, &SG_ERROR_INVALPATH, 0,
                  "'%s' is a reserved filename", buf);
    return -1;
}

/*
  Table for normalizing characters in pathnames.  All paths are folded
  to lower case, and special characters are not allowed.  The list of
  legal characters is very conservative; only alphanumeric characters
  and "-" "_" "." are allowed.
*/
static const unsigned char SG_PATH_NORM[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,'-','.',0,
    '0','1','2','3','4','5','6','7','8','9',0,0,0,0,0,0,
    0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
    'p','q','r','s','t','u','v','w','x','y','z',0,0,0,0,'_',
    0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
    'p','q','r','s','t','u','v','w','x','y','z',0,0,0,0,0,
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
sg_path_norm(char *buf, const char *path, size_t pathlen,
             struct sg_error **err)
{
    char const *ip = path, *ie = path + pathlen;
    char *op = buf, *os;
    unsigned int ci, co;
    int r;

    if (pathlen >= SG_MAX_PATH) {
        sg_path_error(err, "path too long");
        return -1;
    }

    do {
        if (op != buf && op[-1] != '/')
            *op++ = '/';
        os = op;
        while (1) {
            if (ip == ie) {
                co = '\0';
                break;
            }
            ci = (unsigned char) *ip++;
            co = SG_PATH_NORM[ci];
            if (!co) {
                if (ci == '/' || ci == '\\') {
                    co = '/';
                    break;
                } else {
                    sg_path_error(err, "illegal character");
                    return -1;
                }
            }
            *op++ = co;
        }
        if (os == op) {
        } else if (os + 1 == op && op[0] == '.') {
            op = os;
        } else if (os + 2 == op && op[0] == '.' && op[1] == '.') {
            op = os;
            while (op != buf && op[-1] != '/')
                op--;
        } else {
            r = sg_path_checkpart(os, op - os, err);
            if (r)
                return -1;
        }
    } while (co);

    *op = '\0';
    return (int) (op - buf);
}
