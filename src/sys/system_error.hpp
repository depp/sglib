#ifndef SYS_SYSTEM_ERROR_HPP
#define SYS_SYSTEM_ERROR_HPP
#include <stdexcept>
#include <string>

class system_error : public std::exception {
public:
    system_error(int errno) : errno_(errno) { }
    virtual ~system_error() throw ();
    virtual char const *what();
    int errno() const { return errno_; }
private:
    int errno_;
    std::string what_;
};

#endif
