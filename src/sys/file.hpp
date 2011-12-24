#ifndef SYS_FILE_HPP
#define SYS_FILE_HPP
#include "impl/lfile.h"
#include <cstring>

class FBuffer;

class FBufferRef {
    FBuffer *m_ptr;

public:
    explicit FBufferRef(FBuffer *b) throw()
        : m_ptr(b)
    { }

    FBuffer &ref() throw()
    {
        return *m_ptr;
    }
};

// Memory buffer for the contents of a file.  When copied, the buffer
// being copied will be set to NULL.
class FBuffer {
    sg_buffer m_buf;

public:
    FBuffer() throw()
    {
        m_buf.data = 0;
        m_buf.length = 0;
    }

    FBuffer(FBuffer &b) throw()
    {
        std::memcpy(&m_buf, &b.m_buf, sizeof(sg_buffer));
        b.m_buf.data = 0;
        b.m_buf.length = 0;
    }

    FBuffer(FBufferRef r) throw()
    {
        std::memcpy(&m_buf, &r.ref().m_buf, sizeof(sg_buffer));
        r.ref().m_buf.data = 0;
        r.ref().m_buf.length = 0;
    }

    explicit FBuffer(sg_buffer *b)
    {
        std::memcpy(&m_buf, b, sizeof(sg_buffer));
    }

    FBuffer(const char *path, int flags, size_t maxsize);

    FBuffer &operator=(FBuffer &b) throw()
    {
        if (this != &b) {
            clear();
            std::memcpy(&m_buf, &b.m_buf, sizeof(sg_buffer));
            b.m_buf.data = 0;
            b.m_buf.length = 0;
        }
        return *this;
    }

    FBuffer &operator=(FBufferRef r) throw()
    {
        return *this = r.ref();
    }

    void clear() throw()
    {
        sg_buffer_destroy(&m_buf);
        m_buf.data = 0;
        m_buf.length = 0;
    }

    const void *get() const throw()
    {
        return m_buf.data;
    }

    const unsigned char *getUC() const throw()
    {
        return reinterpret_cast<unsigned char *> (m_buf.data);
    }

    size_t size() const throw()
    {
        return m_buf.length;
    }

    operator FBufferRef() throw()
    {
        return FBufferRef(this);
    }
};

class File {
    sg_file *m_ptr;

    explicit File(sg_file *f)
        : m_ptr(f)
    {
        if (m_ptr)
            m_ptr->refcount += 1;
    }

public:
    static const int
        RDONLY = SG_RDONLY,
        WRONLY = SG_WRONLY,
        NO_ARCHIVE = SG_NO_ARCHIVE,
        LOCAL = SG_LOCAL,
        WRITABLE = SG_WRITABLE,
        MAX_PATH = SG_MAX_PATH;

    File(const char *path, int flags);

    File()
        : m_ptr(0)
    { }

    File(const File &f)
        : m_ptr(f.m_ptr)
    {
        if (m_ptr)
            m_ptr->refcount += 1;
    }

    ~File();

    File &operator=(const File &f);

    // Read at most "amt" bytes into "buf" and return the number of
    // bytes read.  Returns 0 if there are no more bytes to read.
    // Throw an exception on error.
    size_t read(void *buf, size_t amt);

    // Write at most "amt" bytes into "buf" and return the number of
    // bytes written.  Throws an exception on error.
    size_t write(const void *buf, size_t amt);

    // Read the entire file, starting from the current offset, into
    // the buffer.  Throw an exception on error.
    FBuffer readall(size_t maxsize);

    // Close the file.
    void close();

    // Get the length.
    int64_t length();
};

#endif
