#ifndef SYS_ERROR_HPP
#define SYS_ERROR_HPP
#include "impl/error.h"
#include <stdexcept>

class error : public std::exception {
    sg_error *m_ptr;

public:
    // Clears the error field
    explicit error(sg_error **e) throw();

    error(const error &e) throw()
        : m_ptr(e.m_ptr)
    {
        m_ptr->refcount++;
    }

    error &operator=(const error &e) throw();

    virtual ~error() throw();

    virtual const char *what() const throw();

    bool is_notfound() const throw()
    {
        return m_ptr->domain == &SG_ERROR_NOTFOUND;
    }
};

#endif
