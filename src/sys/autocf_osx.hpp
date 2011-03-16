#ifndef SYS_AUTOCF_OSX_HPP
#define SYS_AUTOCF_OSX_HPP

template<typename T>
struct AutoCF {
    T ptr_;
    AutoCF() : ptr_(0) { }
    AutoCF(T p) : ptr_(p) { }
    ~AutoCF() { if (ptr_) CFRelease(ptr_); }
    AutoCF &operator=(T p) { if (ptr_) CFRelease(ptr_); ptr_ = p; }
    // T operator*() { return ptr_; }
    operator T() { return ptr_; }
};

#endif
