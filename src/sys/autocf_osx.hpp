#ifndef SYS_AUTOCF_OSX_HPP
#define SYS_AUTOCF_OSX_HPP

/* A reference to a Core Foundation type.  */
template<typename T>
struct AutoCF {
    T ptr_;

    // Null reference.
    AutoCF() throw()
        : ptr_(0)
    { }

    // New reference, takes ownership of the pointer and will call CFRelease.
    explicit AutoCF(T p) throw()
        : ptr_(p)
    { }

    // Copy constructor, retains pointer.
    AutoCF(AutoCF const &o) throw()
        : ptr_(o.ptr_)
    {
        if (ptr_)
            CFRetain(ptr_);
    }

    ~AutoCF() throw()
    {
        if (ptr_)
            CFRelease(ptr_);
    }

    // Set the reference.  Takes ownership of the pointer.
    void set(T p) throw()
    {
        if (ptr_)
            CFRelease(ptr_);
        ptr_ = p;
    }

    // Assignment, retains new pointer and releases old one.
    AutoCF &operator=(AutoCF const &o) throw()
    {
        T po = ptr_, pn = o.ptr_;
        if (pn)
            CFRetain(pn);
        if (po)
            CFRelease(po);
        ptr_ = po;
    }

    // Transparently convert to original type.
    operator T() throw()
    {
        return ptr_;
    }
};

#endif
