#ifndef SYS_SYSTEM_ERROR_HPP
#define SYS_SYSTEM_ERROR_HPP
#include <stdexcept>
#include <string>

class system_error : public std::exception {
public:
    system_error(int errno) throw() : errno_(errno) { }
    virtual ~system_error() throw ();
    virtual char const *what() const throw();
    int errno() const throw() { return errno_; }
private:
    int errno_;
    mutable std::string what_;
};

#endif
