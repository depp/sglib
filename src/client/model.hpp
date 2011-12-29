#ifndef CLIENT_MODEL_HPP
#define CLIENT_MODEL_HPP
// #include "opengl.hpp"
// #include "sys/resource.hpp"
#include "impl/model.h"
#include "sys/sharedref.hpp"
// #include <string>
struct Color;

class Model {
    template<class T> friend class SharedRef;

protected:
    sg_model *m_ptr;

    Model() : m_ptr(0) { }
    Model(sg_model *ptr) : m_ptr(ptr) { }

    void incref() { if (m_ptr) sg_resource_incref(&m_ptr->r); }
    void decref() { if (m_ptr) sg_resource_decref(&m_ptr->r); }
    operator bool() const { return m_ptr != 0; }

public:
    typedef SharedRef<Model> Ref;

    void draw(Color tcolor, Color lcolor) const;

    static Ref file(const char *path);
    static Ref mstatic(sg_model_static_t which);
    static Ref pyramid() { return mstatic(SG_MODEL_PYRAMID); }
    static Ref cube() { return mstatic(SG_MODEL_CUBE); }
};

#endif
