#ifndef GRAPHICS_MODEL_HPP
#define GRAPHICS_MODEL_HPP
#include "opengl.hpp"
#include "sys/resource.hpp"
#include "sys/sharedref.hpp"
#include <string>
struct Color;

class Model : private Resource {
public:
    template<class T> friend class SharedRef;
    typedef SharedRef<Model> Ref;

    static Model kCube, kPyramid;

    virtual ~Model();
    static Ref open(std::string const &path);

    void draw(const Color tcolor, const Color lcolor) const;
    virtual std::string name() const;
    std::string const &path() const { return path_; }

private:
    virtual void loadResource();
    virtual void unloadResource();

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
};

#endif
