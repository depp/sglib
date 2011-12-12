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
    // "x = x" is safe.
    AutoCF &operator=(AutoCF const &o) throw()
    {
        T po = ptr_, pn = o.ptr_;
        if (pn)
            CFRetain(pn);
        if (po)
            CFRelease(po);
        ptr_ = po;
        return *this;
    }

    // Transparently convert to original type.
    operator T() throw()
    {
        return ptr_;
    }

    // Relinquish ownership of the pointer and return it.
    T release() throw()
    {
        T p = ptr_;
        ptr_ = 0;
        return p;
    }

    // Release the contained pointer.
    void clear() throw()
    {
        if (ptr_) {
            CFRelease(ptr_);
            ptr_ = 0;
        }
    }

    // Move (not copy) a pointer from one ref to another.
    // "x.moveFrom(x)" is not safe.
    void moveFrom(AutoCF &o) throw()
    {
        T pn = o.ptr_, po = ptr_;
        o.ptr_ = 0;
        if (po)
            CFRelease(po);
        ptr_ = pn;
    }

    operator bool() throw()
    {
        return ptr_ != 0;
    }
};

#endif
