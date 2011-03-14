#ifndef SYS_SHAREDREF_HPP
#define SYS_SHAREDREF_HPP

template<class T>
class SharedRef {
    template<class TO> friend class SharedRef;
public:
    SharedRef() throw()
        : ptr_(0)
    { }

    ~SharedRef() throw()
    {
        if (ptr_)
            ptr_->decref();
    }

    SharedRef(T *p) throw()
        : ptr_(p)
    {
        if (ptr_)
            p->incref();
    }

    SharedRef(SharedRef const &r) throw()
        : ptr_(r.ptr_)
    {
        if (ptr_)
            ptr_->incref();
    }

    template<class TO>
    SharedRef(SharedRef<TO> const &r) throw()
        : ptr_(r.ptr_)
    {
        if (ptr_)
            ptr_->incref();
    }

    SharedRef &operator=(SharedRef const &r) throw()
    {
        T *po = ptr_, *pn = r.ptr_;
        if (pn)
            pn->incref();
        if (po)
            po->decref();
        ptr_ = pn;
        return *this;
    }

    template <class TO>
    SharedRef &operator=(SharedRef<TO> const &r) throw()
    {
        T *po = ptr_, *pn = r.ptr_;
        if (pn)
            pn->incref();
        if (po)
            po->decref();
        ptr_ = pn;
        return *this;
    }

    T* operator->() const throw()
    {
        return ptr_;
    }

    operator bool() const
    {
        return ptr_;
    }

    void clear()
    {
        if (ptr_)
            ptr_->decref();
        ptr_ = 0;
    }

    T* get() const
    {
        return ptr_;
    }

private:
    T *ptr_;
};

#endif
