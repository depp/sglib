#include "path.hpp"
#include "ifile.hpp"
#include "error_win.hpp"
#include <Windows.h>
#include <assert.h>

static std::string gExeDir;

void Path::init()
{
    unsigned i;
    DWORD r;
    char buf[MAX_PATH];
    r = GetModuleFileName(NULL, buf, MAX_PATH);
    if (r == 0)
        throw error_win(GetLastError());
    if (r == MAX_PATH)
        throw error_win(ERROR_INSUFFICIENT_BUFFER);
    for (i = r; i > 0 && buf[i - 1] != '\\'; --i) { }
    assert(i > 0);
    gExeDir = std::string(buf, i);
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

static IFile *tryOpenFile(std::string const &path)
{
    HANDLE h = CreateFile(path.c_str(), FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        if (e == ERROR_FILE_NOT_FOUND)
            return NULL;
        throw error_win(e);
    }
    try {
        return new IFileWin(h);
    } catch (std::exception &) {
        CloseHandle(h);
        throw;
    }
}

IFile *Path::openIFile(std::string const &path)
{
    IFile *f;
    std::string s;
    std::string::const_iterator p, e;
    char buf[MAX_PATH], c;
    unsigned i;
    if (path.length() >= MAX_PATH - 1)
        goto not_found;
    buf[0] = '\\';
    for (i = 1, p = path.begin(), e = path.end(); p != e; ++p) {
        c = *p;
        if (c == '/')
            c = '\\';
        buf[i] = c;
    }
    s = std::string(buf, i);
    f = tryOpenFile(gExeDir + s);
    if (!f)
        goto not_found;
    return f;

not_found:
    throw error_win(ERROR_FILE_NOT_FOUND);
}
