#include "ifile.hpp"
#include <limits>

IFile::~IFile() { }

Buffer IFile::readall_dynamic()
{
    Buffer buf(1024);
    size_t sz = 0, amt;
    while ((amt = read(buf.getUC() + sz, buf.size() - sz))) {
        sz += amt;
        if (sz == buf.size()) {
            if (sz > std::numeric_limits<size_t>::max() / 2)
                throw std::bad_alloc();
            buf.resize(sz * 2);
        }
    }
    if (sz < buf.size())
        buf.resize(sz);
    return buf;
}

Buffer IFile::readall_static(int64_t size)
{
    if (size == 0)
        return Buffer();
    if (size < 0 ||
        static_cast<uint64_t>(size) > std::numeric_limits<size_t>::max())
        throw std::bad_alloc();
    size_t sz = size, p = 0, amt;
    Buffer buf(sz);
    while ((amt = read(buf.getUC() + p, sz - p))) {
        p += amt;
        if (p == sz)
            break;
    }
    return buf;
}
