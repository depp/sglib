/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_ERROR_HPP
#define SGPP_ERROR_HPP
#include "sg/error.h"
#include <stdexcept>

class error : public std::exception {
    sg_error *m_ptr;

public:
    // Clears the error field
    explicit error(sg_error **e) throw();

    error(const error &e) throw()
        : exception(e), m_ptr(e.m_ptr)
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

    sg_error *get() const
    {
        return m_ptr;
    }
};

#endif
