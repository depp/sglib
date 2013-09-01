/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_BUFFER_HPP
#define SGPP_BUFFER_HPP
#include <stdlib.h>
#include <memory>

class Buffer;

/* Ignore the "BufferRef" class.  */
class BufferRef {
public:
    explicit BufferRef(Buffer *b) throw() : p_(b) { }
    Buffer &ref() throw() { return *p_; }
private:
    Buffer *p_;
};

/* Memory buffer.  Like auto_ptr, when copied, the buffer being copied
   will be set to NULL.  */
class Buffer {
public:
    Buffer() throw() : ptr_(0), len_(0) { }
    Buffer(size_t sz) : ptr_(0), len_(0) { resize(sz); }
    Buffer(void *ptr, size_t len) throw() : ptr_(ptr), len_(len) { }
    Buffer(Buffer &b) throw() : ptr_(b.ptr_), len_(b.len_) { b.release(); }
    Buffer(BufferRef r) throw()
    {
        void *p = r.ref().ptr_;
        size_t l = r.ref().len_;
        r.ref().release();
        ptr_ = p;
        len_ = l;
    }
    ~Buffer() throw() { if (ptr_) free(ptr_); }

    Buffer &operator=(Buffer &b) throw()
    {
        if (this != &b) {
            clear();
            ptr_ = b.ptr_;
            len_ = b.len_;
            b.release();
        }
        return *this;
    }

    Buffer &operator=(BufferRef r) throw()
    {
        if (this != &r.ref()) {
            void *p = r.ref().ptr_;
            size_t l = r.ref().len_;
            r.ref().release();
            clear();
            ptr_ = p;
            len_ = l;
        }
        return *this;
    }

    void clear() throw()
    {
        if (ptr_) {
            free(ptr_);
            ptr_ = 0;
            len_ = 0;
        }
    }

    void resize(size_t sz)
    {
        void *p = realloc(ptr_, sz);
        if (sz && !p)
            throw std::bad_alloc();
        ptr_ = p;
        len_ = sz;
    }

    void release() throw()
    {
        ptr_ = 0;
        len_ = 0;
    }

    void *get() const throw() { return ptr_; }
    unsigned char *getUC() const throw()
    { return reinterpret_cast<unsigned char *>(ptr_); }
    size_t size() const { return len_; }

    operator BufferRef() throw() { return BufferRef(this); }

private:
    void *ptr_;
    size_t len_;
};

#endif
