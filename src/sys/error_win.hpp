#ifndef SYS_ERROR_WIN_HPP
#define SYS_ERROR_WIN_HPP
#include <stdexcept>
#include <string>
 
class error_win : public std::exception {
public:
    explicit error_win(int code) throw() : code_(code) { }
    virtual ~error_win() throw ();
    virtual char const *what() const throw();
    int code() const throw() { return code_; }
private:
    int code_;
    mutable std::string what_;
};

#endif
