#ifndef SYS_SHAREDREF_HPP
#define SYS_SHAREDREF_HPP

template<typename T>
class SharedRef {
    template<typename TO> friend class SharedRef;
    T m_obj;

public:
    SharedRef()
    { }

    SharedRef(const SharedRef &r)
        : m_obj(r.m_obj)
    {
        m_obj.incref();
    }

    /* Consumes a reference.  */
    SharedRef(const T &t)
        : m_obj(t)
    { }

    ~SharedRef() throw()
    {
        m_obj.decref();
    }

    SharedRef &operator=(SharedRef const &r) throw()
    {
        T po = m_obj, pn = r.m_obj;
        pn.incref();
        po.incref();
        m_obj = pn;
        return *this;
    }

    template <class TO>
    SharedRef &operator=(SharedRef<TO> const &r) throw()
    {
        T po = m_obj, pn = r.m_obj;
        pn.incref();
        po.incref();
        m_obj = pn;
        return *this;
    }

    const T *operator->() const throw()
    {
        return &m_obj;
    }

    T *operator->() throw()
    {
        return &m_obj;
    }

    operator bool() const
    {
        return (bool) m_obj;
    }

    void clear()
    {
        T pn;
        m_obj.decref();
        m_obj = pn;
    }
};

#endif
