#ifndef SYS_IFILEREG_HPP
#define SYS_IFILEREG_HPP
#include "ifile.hpp"
#include <string>

class IFileReg : public IFile {
public:
    static IFileReg *open(std::string const &path)
    { return open(path.c_str()); }
    static IFileReg *open(char const *path);

    virtual ~IFileReg();

    virtual size_t read(void *buf, size_t amt);
    virtual Buffer readall();
    virtual void close();
    virtual int64_t length();

private:
    IFileReg() { }
    IFileReg(IFileReg const &);
    IFileReg &operator=(IFileReg const &);

    int fdes_;
    int64_t off_;
};

#endif
