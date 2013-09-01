/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_FILE_HPP
#define SGPP_FILE_HPP
#include "sg/file.h"
#include <cstring>

class FBuffer;

// Memory buffer for the contents of a file.
class FBuffer {
    sg_buffer *m_buf;

public:
    FBuffer() throw()
    {
        m_buf = 0;
    }

    FBuffer(const FBuffer &b) throw()
        : m_buf(b.m_buf)
    {
        if (m_buf)
            sg_buffer_incref(m_buf);
    }

    /* Assumes ownership of the reference.  */
    explicit FBuffer(sg_buffer *b)
        : m_buf(b)
    { }

    FBuffer(const char *path, int flags, const char *extensions,
            size_t maxsize);

    ~FBuffer()
    {
        if (m_buf)
            sg_buffer_decref(m_buf);
    }

    FBuffer &operator=(FBuffer &b) throw()
    {
        if (m_buf != b.m_buf) {
            sg_buffer_decref(m_buf);
            sg_buffer_incref(b.m_buf);
            m_buf = b.m_buf;
        }
        return *this;
    }

    void clear() throw()
    {
        if (m_buf) {
            sg_buffer_decref(m_buf);
            m_buf = 0;
        }
    }

    const void *get() const throw()
    {
        return m_buf ? m_buf->data : 0;
    }

    const unsigned char *getUC() const throw()
    {
        return reinterpret_cast<const unsigned char *> (get());
    }

    size_t size() const throw()
    {
        return m_buf ? m_buf->length : 0;
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
