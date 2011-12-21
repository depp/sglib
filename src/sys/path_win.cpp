#include "win/defs.h"
#include "path.hpp"
#include "ifile.hpp"
#include "error_win.hpp"
#include <Windows.h>
#include <assert.h>

struct path_dir {
    LPWSTR path;
    unsigned len;
};

static path_dir gDirs[2];

void path_init(exe_options *opts)
{
    unsigned i;
    DWORD r;
    wchar_t buf[MAX_PATH], *p, *s;

    // Get default path: the executable directory
    r = GetModuleFileName(NULL, buf, MAX_PATH);
    if (r == 0)
        goto error;
    if (r == MAX_PATH)
        goto error;
    for (i = r; i > 0 && buf[i - 1] != L'\\'; --i) { }
    if (i <= 1)
        goto error;
    i--;
    p = (wchar_t *) malloc((i + 5) * sizeof(wchar_t));
    if (!p)
        goto error;
    wmemcpy(p, buf, i);
    wmemcpy(p + i, L"\\data", 5);
    gDirs[0].path = p;
    gDirs[0].len = i + 5;

    // Get the alternate path specified on the command line
    s = opts->alt_data_dir;
    if (s && *s) {
        i = wcslen(s);
        if (s[i-1] == '\\')
            i--;
        p = (wchar_t *) malloc(i * sizeof(wchar_t));
        if (!p)
            goto error;
        wmemcpy(p, s, i);
        gDirs[1].path = p;
        gDirs[1].len = i;
    }

    return;

error:
    OutputDebugStringA("path_init failed");
    abort();
}

class IFileWin : public IFile {
public:
    IFileWin(HANDLE h)
        : hdl_(h), off_(0)
    { }

    virtual ~IFileWin();

    virtual size_t read(void *buf, size_t amt);
    virtual Buffer readall();
    virtual void close();
    virtual int64_t length();

private:
    IFileWin(IFileWin const &);
    IFileWin &operator=(IFileWin const &);
    
    HANDLE hdl_;
    int64_t off_;
};

IFileWin::~IFileWin()
{
    if (hdl_ != INVALID_HANDLE_VALUE)
        CloseHandle(hdl_);
}

size_t IFileWin::read(void *buf, size_t amt)
{
    DWORD ramt;
    if (!ReadFile(hdl_, buf, amt, &ramt, NULL))
        throw error_win(GetLastError());
    off_ += ramt;
    return ramt;
}

Buffer IFileWin::readall()
{
    return readall_static(length() - off_);
}

void IFileWin::close()
{
    if (hdl_ != INVALID_HANDLE_VALUE) {
        CloseHandle(hdl_);
        hdl_ = INVALID_HANDLE_VALUE;
    }
}

int64_t IFileWin::length()
{
    BY_HANDLE_FILE_INFORMATION ifo;
    if (!GetFileInformationByHandle(hdl_, &ifo))
        throw error_win(GetLastError());
    return ((int64_t) ifo.nFileSizeHigh << 32) | ifo.nFileSizeLow;
}

IFile *Path::openIFile(std::string const &path)
{
    wchar_t buf[MAX_PATH];
    unsigned ll, i, off, off2;
    char c;
    ll = path.size();
    if (ll > MAX_PATH - 2)
        goto not_found;
    off = MAX_PATH - 1 - ll;
    for (i = 0; i < ll; ++i) {
        c = path[i];
        if (c < 0x20 || c > 0x7e)
            goto not_found;
        if (c == '/')
            c = '\\';
        buf[off + i] = c;
    }
    off--;
    buf[off] = '\\';
    buf[MAX_PATH - 1] = '\0';
    for (i = 0; i < 2; ++i) {
        if (!gDirs[i].len || gDirs[i].len > off)
            continue;
        off2 = off - gDirs[i].len;
        wmemcpy(buf + off2, gDirs[i].path, gDirs[i].len);
        HANDLE h = CreateFileW(buf + off2, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            DWORD e = GetLastError();
            if (e != ERROR_FILE_NOT_FOUND && e != ERROR_PATH_NOT_FOUND)
                throw error_win(e);
        } else {
            try {
                return new IFileWin(h);
            } catch (std::exception &) {
                CloseHandle(h);
                throw;
            }
        }
    }

not_found:
    throw file_not_found();
}

FILE *Path::openOFile(std::string const &path)
{
    abort();
    return NULL;
}
