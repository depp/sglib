/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sgpp/error.hpp"
#include <cstdlib>
#include <assert.h>

error::error(sg_error **e) throw()
    : m_ptr(*e)
{
    assert(m_ptr != NULL);
    *e = NULL;
}

error &error::operator=(const error &e) throw()
{
    sg_error *optr = m_ptr;
    m_ptr = e.m_ptr;
    m_ptr->refcount++;
    if (--optr->refcount == 0)
        std::free(optr);
    return *this;
}

error::~error() throw()
{
    if (--m_ptr->refcount == 0)
        std::free(m_ptr);
}

const char *error::what() const throw()
{
    return m_ptr->msg;
}
