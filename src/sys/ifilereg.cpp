#include "ifilereg.hpp"
#include "system_error.hpp"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

IFileReg *IFileReg::open(char const *path)
{
    int fdes = ::open(path, O_RDONLY);
    IFileReg *f;
    if (fdes < 0)
        throw system_error(errno);
    try {
        f = new IFileReg;
    } catch (std::exception const &e) {
        ::close(fdes);
        throw;
    }
    f->fdes_ = fdes;
    f->off_ = 0;
    return f;
}

IFileReg::~IFileReg()
{
    if (fdes_ >= 0)
        ::close(fdes_);
}

size_t IFileReg::read(void *buf, size_t amt)
{
    ssize_t r;
    while (1) {
        r = ::read(fdes_, buf, amt);
        if (r < 0) {
            int e = errno;
            if (e == EINTR)
                continue;
            throw system_error(e);
        } else
            break;
    }
    off_ += r;
    return r;
}

Buffer IFileReg::readall()
{
    return readall_static(length() - off_);
}

void IFileReg::close()
{
    if (fdes_ < 0)
        return;
    ::close(fdes_);
    fdes_ = -1;
}

int64_t IFileReg::length()
{
    struct stat st;
    int r = ::fstat(fdes_, &st);
    if (r < 0)
        throw system_error(errno);
    return st.st_size;
}
