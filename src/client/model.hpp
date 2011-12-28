#ifndef CLIENT_MODEL_HPP
#define CLIENT_MODEL_HPP
// #include "opengl.hpp"
// #include "sys/resource.hpp"
#include "sys/sharedref.hpp"
// #include <string>
struct Color;

class Model {
    template<class T> friend class SharedRef;

protected:
    void *m_ptr;

    Model() : m_ptr(0) { }
    Model(void *ptr) : m_ptr(ptr) { }

    void incref() { }
    void decref() { }
    operator bool() const { return m_ptr != 0; }

public:
    typedef SharedRef<Model> Ref;

    void draw(Color tcolor, Color lcolor) const;

    static Ref file(const char *path);
    static Ref pyramid();
    static Ref cube();
};

/*
    Model(std::string const &path);
    Model(double scale,
          unsigned int vcount, short const vdata[][3],
          unsigned int tcount, unsigned short const tdata[][3],
          unsigned int lcount, unsigned short const ldata[][2])
        : Resource(false),
          path_(), data_(0), datalen_(0), scale_(scale),
          vtype_(GL_SHORT), vcount_(vcount), vdata_(vdata),
          ttype_(GL_UNSIGNED_SHORT), tcount_(tcount), tdata_(tdata),
          ltype_(GL_UNSIGNED_SHORT), lcount_(lcount), ldata_(ldata)
    {
        setLoaded(true);
    }

    std::string path_;
    void *data_;
    unsigned int datalen_;

    double scale_;

    unsigned int vtype_;
    unsigned int vcount_;
    void const *vdata_;

    unsigned int ttype_;
    unsigned int tcount_;
    void const *tdata_;

    unsigned int ltype_;
    unsigned int lcount_;
    void const *ldata_;
*/

#endif
