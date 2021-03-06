/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "config.h"
#include "sg/defs.h"
#include "sg/log.h"
#include "sg/version.h"

#include <string.h>
#include <stdio.h>

void
sg_version_lib(const char *libname,
               const char *compileversion, const char *runversion)
{
    /*
    if (SG_LOG_DEBUG >= lp->level && compileversion && runversion &&
        strcmp(compileversion, runversion))
        sg_logf(lp, SG_LOG_INFO, "%s: version %s (compiled against %s)",
                libname, compileversion, runversion);
    else
    */
    sg_logf(SG_LOG_INFO, "%s: version %s", libname,
            runversion ? runversion : compileversion);
}

#if defined(__linux__)

#include <dirent.h>

static const char DISTROS[] =
    "arch\0centos\0debian\0fedora\0gentoo\0"
    "mandrake\0mandriva\0redhat\0rocks\0slackware\0"
    "SuSE\0turbolinux\0UnitedLinux\0yellowdog\0";

static void
sg_version_os_impl(char *verbuf, size_t bufsz)
{
    DIR *d;
    int r;
    struct dirent ent, *result;
    const char *p, *q, *dist;
    size_t plen, qlen;
    FILE *f;
    char buf[256], *s;

    snprintf(verbuf, bufsz, "Linux/unknown");

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
    f = fopen(buf, "re");
    if (!f) {
        return;
    }
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
    snprintf(verbuf, bufsz, "Linux/%s", dist);
}

void
sg_version_machineid(char *buf, size_t bufsz)
{
    (void) bufsz;
    buf[0] = '\0';
}

#elif defined(__APPLE__)
#include <CoreServices/CoreServices.h>

static void
sg_version_os_impl(char *verbuf, size_t bufsz)
{
    SInt32 major, minor, bugfix;
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    Gestalt(gestaltSystemVersionBugFix, &bugfix);
    snprintf(verbuf, bufsz, "OS X/%d.%d.%d", major, minor, bugfix);
}

void
sg_version_machineid(char *buf, size_t bufsz)
{
    (void) bufsz;
    buf[0] = '\0';
}

#elif defined(_WIN32)
#include <Windows.h>

/*
To get the actual system version, we ask for the version of kernel32.dll.
If we call GetVersionEx() then Windows will lie to us.
http://msdn.microsoft.com/en-us/library/windows/desktop/ms724429(v=vs.85).aspx
*/

#pragma comment(lib, "version.lib")

static void
sg_version_os_impl(char *verbuf, size_t bufsz)
{
    static const wchar_t kernel32[] = L"\\kernel32.dll";
    wchar_t *path = NULL;
    void *ver = NULL, *block;
    UINT n;
    BOOL r;
    DWORD versz, blocksz;
    VS_FIXEDFILEINFO *vinfo;

    path = malloc(sizeof(*path) * MAX_PATH);
    if (!path)
        goto unknown;

    n = GetSystemDirectory(path, MAX_PATH);
    if (n >= MAX_PATH || n == 0 ||
        n > MAX_PATH - sizeof(kernel32) / sizeof(*kernel32))
        goto unknown;
    memcpy(path + n, kernel32, sizeof(kernel32));

    versz = GetFileVersionInfoSize(path, NULL);
    if (versz == 0)
        goto unknown;
    ver = malloc(versz);
    if (!ver)
        goto unknown;
    r = GetFileVersionInfo(path, 0, versz, ver);
    if (!r)
        goto unknown;
    r = VerQueryValue(ver, L"\\", &block, &blocksz);
    if (!r || blocksz < sizeof(VS_FIXEDFILEINFO))
        goto unknown;
    vinfo = (VS_FIXEDFILEINFO *) block;
    _snprintf_s(
        verbuf, bufsz, _TRUNCATE,
        "Windows/%d.%d.%d",
        (int) HIWORD(vinfo->dwProductVersionMS),
        (int) LOWORD(vinfo->dwProductVersionMS),
        (int) HIWORD(vinfo->dwProductVersionLS));
    free(path);
    free(ver);
    return;

unknown:
    _snprintf_s(verbuf, bufsz, _TRUNCATE, "Windows/unknown");
    free(path);
    free(ver);
}

void
sg_version_machineid(char *buf, size_t bufsz)
{
    (void) bufsz;
    buf[0] = '\0';
}

#else

static void
sg_version_os_impl(char *verbuf, size_t bufsz)
{
    snprintf(verbuf, bufsz, "Unknown");
}

void
sg_version_machineid(char *buf, size_t bufsz)
{
    (void) bufsz;
    buf[0] = '\0';
}

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
sg_version_os(char *verbuf, size_t bufsz)
{
    char tmp[64];
    sg_version_os_impl(tmp, sizeof(tmp));
#if defined _WIN32
    _snprintf_s(verbuf, bufsz, _TRUNCATE, "%s (%s)", tmp, ARCH);
#else
    snprintf(verbuf, bufsz, "%s (%s)", tmp, ARCH);
#endif
}

void
sg_version_print(void)
{
    char buf[256];
    sg_logf(SG_LOG_INFO, "App version: %s (%s)",
            SG_APP_VERSION, SG_APP_COMMIT);
    sg_logf(SG_LOG_INFO, "SGLib version: %s (%s)",
            SG_SG_VERSION, SG_SG_COMMIT);
    sg_logf(SG_LOG_INFO, "Compiler: " COMPILER);
    sg_version_os(buf, sizeof(buf));
    sg_logs(SG_LOG_INFO, buf);
    sg_version_platform();

#if defined USE_PNG_LIBPNG
    sg_version_libjpeg();
#endif

#if defined USE_JPEG_LIBJPEG
    sg_version_libpng();
#endif
}
