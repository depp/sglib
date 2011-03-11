#ifndef SYS_IFILE_HPP
#define SYS_IFILE_HPP
#include <stddef.h>
#include <stdint.h>
#include "buffer.hpp"

class IFile {
public:
    virtual ~IFile();

    // Read at most "amt" bytes into "buf" and return the number of
    // bytes read.  Returns 0 if there are no more bytes to read.
    // Throw an exception on error.
    virtual size_t read(void *buf, size_t amt) = 0;

    // Read the entire file, starting from the current offset, into
    // the buffer.  Throw an exception on error.
    virtual Buffer readall() = 0;

    // Close the file.
    virtual void close() = 0;

    // Get the length
    virtual int64_t length() = 0;

protected:
    // Read the entire file by repeatedly calling "read" until it
    // returns zero.  Requires some extra overhead.
    Buffer readall_dynamic();

    // Read the entire file based on the known number of bytes
    // remaining.
    Buffer readall_static(int64_t size);
};

#endif
