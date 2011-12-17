#ifndef SYS_SYSTEM_ERROR_HPP
#define SYS_SYSTEM_ERROR_HPP
#include <stdexcept>
#include <string>

class system_error : public std::exception {
public:
    system_error(int code) throw() : code_(code) { }
    virtual ~system_error() throw ();
    virtual char const *what() const throw();
    int code() const throw() { return code_; }
private:
    int code_;
    mutable std::string what_;
};

#endif
