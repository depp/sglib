/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "sgpp/file.hpp"
#include "sgpp/error.hpp"
#include <stdio.h>
#include <string.h>

FBuffer::FBuffer(const char *path, int flags, const char *extensions,
                 size_t maxsize)
    : m_buf(0)
{
    sg_buffer *b;
    struct sg_error *e = NULL;
    b = sg_file_get(path, strlen(path), flags, extensions, maxsize, &e);
    if (!b)
        throw error(&e);
    m_buf = b;
}

File::File(const char *path, int flags)
    : m_ptr(0)
{
    sg_error *e = NULL;
    m_ptr = sg_file_open(path, strlen(path), flags, NULL, &e);
    if (!m_ptr)
        throw error(&e);
}

File::~File()
{
    if (m_ptr && --m_ptr->refcount == 0)
        m_ptr->free(m_ptr);
}

File &File::operator=(const File &f)
{
    sg_file *optr = m_ptr;
    m_ptr = f.m_ptr;
    if (m_ptr)
        m_ptr->refcount += 1;
    if (optr && --optr->refcount == 0)
        optr->free(optr);
    return *this;
}

size_t File::read(void *buf, size_t amt)
{
    int r = m_ptr->read(m_ptr, buf, amt);
    if (r < 0)
        throw error(&m_ptr->err);
    return r;
}

size_t File::write(const void *buf, size_t amt)
{
    int r = m_ptr->write(m_ptr, buf, amt);
    if (r < 0)
        throw error(&m_ptr->err);
    return r;
}

FBuffer File::readall(size_t maxsize)
{
    sg_buffer *b;
    b = sg_file_readall(m_ptr, maxsize);
    if (!b)
        throw error(&m_ptr->err);
    return FBuffer(b);
}

void File::close()
{
    int r;
    if (!m_ptr)
        return;
    r = m_ptr->close(m_ptr);
    if (r) {
        error e(&m_ptr->err);
        if (--m_ptr->refcount)
            m_ptr->free(m_ptr);
        m_ptr = NULL;
        throw e;
    }
    if (!--m_ptr->refcount)
        m_ptr->free(m_ptr);
    m_ptr = NULL;
}

int64_t File::length()
{
    int64_t r = m_ptr->length(m_ptr);
    if (r < 0)
        throw error(&m_ptr->err);
    return r;
}
