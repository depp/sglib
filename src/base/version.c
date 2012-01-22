#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "entry.h"
#include "log.h"
#include "version.h"

#include <string.h>

void
sg_version_lib(struct sg_logger *lp, const char *libname,
               const char *compileversion, const char *runversion)
{
    if (LOG_DEBUG >= lp->level && compileversion && runversion &&
        strcmp(compileversion, runversion))
        sg_logf(lp, LOG_INFO, "%s: version %s (compiled against %s)",
                libname, compileversion, runversion);
    else
        sg_logf(lp, LOG_INFO, "%s: version %s", libname,
                runversion ? runversion : compileversion);
}

#if defined(__linux__)

#include <dirent.h>
#include <stdio.h>

static const char DISTROS[] =
    "arch\0centos\0debian\0fedora\0gentoo\0"
    "mandrake\0mandriva\0redhat\0rocks\0slackware\0"
    "SuSE\0turbolinux\0UnitedLinux\0yellowdog\0";

static void
sg_version_linuxdistro(struct sg_logger *lp)
{
    DIR *d;
    int r;
    struct dirent ent, *result;
    const char *p, *q, *dist;
    size_t plen, qlen;
    FILE *f;
    char buf[256], *s;

    /* Scan for a file of the form x[-_](release|version).
       Check that the "x" matches a distro name above.  */
    d = opendir("/etc");
    if (!d)
        return;
next_file:
    r = readdir_r(d, &ent, &result);
    if (r || !result) {
        closedir(d);
        return;
    }
    p = ent.d_name;
    while (1) {
        if ((*p >= 'a' && *p <= 'z') ||
            (*p >= 'A' && *p <= 'Z'))
            p++;
        else if (*p == '-' || *p == '_')
            break;
        else
            goto next_file;
    }
    plen = p - ent.d_name;
    p++;
    if (strcmp(p, "release") && strcmp(p, "version"))
        goto next_file;
    q = DISTROS;
    while (1) {
        qlen = strlen(q);
        if (!qlen)
            goto next_file;
        if (plen == qlen && !memcmp(ent.d_name, q, qlen))
            break;
        q += qlen + 1;
    }
    closedir(d);

    /* Read the first line of the version file.  */
    snprintf(buf, sizeof(buf), "/etc/%s", ent.d_name);
    f = fopen(buf, "r");
    if (!f)
        return;
    memcpy(buf, q, qlen);
    buf[qlen] = ' ';
    s = fgets(buf + qlen + 1, sizeof(buf) - qlen - 1, f);
    fclose(f);
    if (!s)
        return;
    s = strchr(buf + qlen + 1, '\n');
    if (s)
        *s = '\0';
    /* If the line starts with a version number then it probably
       doesn't contain the distro name so we derive it from the file
       name.  */
    if (buf[qlen + 1] >= '0' && buf[qlen + 1] <= '9')
        dist = buf;
    else
        dist = buf + qlen + 1;
    sg_logf(lp, LOG_INFO, "Linux distribution: %s", dist);
}

static void
sg_version_os(struct sg_logger *lp)
{
    sg_version_linuxdistro(lp);
}

#elif defined(__APPLE__)
#include <CoreServices/CoreServices.h>

static void
sg_version_os(struct sg_logger *lp)
{
    SInt32 major, minor, bugfix;
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    Gestalt(gestaltSystemVersionBugFix, &bugfix);
    sg_logf(lp, LOG_INFO, "Mac OS X: %d.%d.%d", major, minor, bugfix);
}

#else

#define sg_version_os(x) (void)0

#endif

#if defined(HAVE_LIBJPEG)
#include <jpeglib.h>
static void
sg_version_libjpeg(struct sg_logger *lp)
{
    char vers[8];
    int major, minor;
#ifndef JPEG_LIB_VERSION_MINOR
    major = JPEG_LIB_VERSION / 10;
    minor = JPEG_LIB_VERSION % 10;
#else
    major = JPEG_LIB_VERSION_MAJOR;
    minor = JPEG_LIB_VERSION_MINOR;
#endif
    if (minor)
        snprintf(vers, sizeof(vers), "%d%c", major, minor + 'a');
    else
        snprintf(vers, sizeof(vers), "%d", major);
    sg_version_lib(lp, "LibJPEG", vers, NULL);
}
#else
#define sg_version_libjpeg(x) (void)0
#endif

#if defined(HAVE_LIBPNG)
#include <png.h>
static void
sg_version_libpng(struct sg_logger *lp)
{
    int v = png_access_version_number(), maj, min, mic;
    char vers[16];
    min = v / 100;
    mic = v % 100;
    maj = min / 100;
    min = min % 100;
    snprintf(vers, sizeof(vers), "%d.%d.%d", maj, min, mic);
    sg_version_lib(lp, "LibPNG", PNG_LIBPNG_VER_STRING, vers);
}
#else
#define sg_version_libpng(x) (void)0
#endif

#if defined(HAVE_PANGOCAIRO)
#include <pango/pango.h>
static void
sg_version_pango(struct sg_logger *lp)
{
    sg_version_lib(lp, "Pango", PANGO_VERSION_STRING,
                   pango_version_string());
}
#else
#define sg_version_pango(x) (void)0
#endif

#if defined(_MSC_VER)
#define COMPILER "MSC %d", _MSC_VER
#elif defined(__clang__)
#define COMPILER "Clang " __VERSION__
#elif defined(__GNUC__)
#define COMPILER "GCC " __VERSION__
#else
#define COMPILER "unknown"
#endif

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH "x64"
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH "x86"
#elif defined(__ppc64__)
#define ARCH "ppc64"
#elif defined(__ppc__)
#define ARCH "ppc"
#else
#define ARCH "unknown"
#endif

void
sg_version_print(void)
{
    struct sg_logger *log = sg_logger_get("init");
    sg_logf(log, LOG_INFO, "Version: %s", SG_VERSION);
    sg_logf(log, LOG_INFO, "Compiler: " COMPILER);
    sg_logf(log, LOG_INFO, "Architecture: " ARCH);
    sg_version_os(log);
    sg_platform_version(log);
    sg_version_libjpeg(log);
    sg_version_libpng(log);
    sg_version_pango(log);
}
