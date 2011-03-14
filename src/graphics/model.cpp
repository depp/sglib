#include "model.hpp"
#include "color.hpp"
#include <set>
#include <memory>
#include <stdlib.h>

static const short kCubeVertices[8][3] = {
    { -1, -1, -1 }, { -1, -1,  1 }, { -1,  1, -1 }, { -1,  1,  1 },
    {  1, -1, -1 }, {  1, -1,  1 }, {  1,  1, -1 }, {  1,  1,  1 }
};

static const unsigned short kCubeTris[12][3] = {
    { 1, 3, 2 }, { 2, 0, 1 },
    { 0, 2, 6 }, { 6, 4, 0 },
    { 4, 6, 7 }, { 7, 5, 4 },
    { 1, 5, 7 }, { 7, 3, 1 },
    { 2, 3, 7 }, { 7, 6, 2 },
    { 0, 4, 5 }, { 5, 1, 0 }
};

static const unsigned short kCubeLines[12][2] = {
    { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
    { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
    { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
};

Model Model::kCube
(1.0, 8, kCubeVertices, 12, kCubeTris, 12, kCubeLines);

static const short kPyramidVertices[8][3] = {
    { -1, -1, -1 }, { -1,  1, -1 }, {  1, -1, -1 }, {  1,  1, -1 },
    {  0,  0,  1 }
};

static const unsigned short kPyramidTris[6][3] = {
    { 0, 1, 3 }, { 3, 2, 0 },
    { 0, 4, 1 }, { 1, 4, 3 },
    { 3, 4, 2 }, { 2, 4, 0 }
};

static const unsigned short kPyramidLines[8][2] = {
    { 0, 1 }, { 2, 3 }, { 0, 2 }, { 1, 3 },
    { 0, 4 }, { 1, 4 }, { 2, 4 }, { 3, 4 }
};

Model Model::kPyramid
(1.0, 5, kPyramidVertices, 6, kPyramidTris, 8, kPyramidLines);

struct ModelCompare {
    bool operator()(Model *x, Model *y)
    { return x->path() < y->path(); }
};

typedef std::set<Model *, ModelCompare> ModelSet;
static ModelSet modelFiles;

Model::Ref Model::open(std::string const &path)
{
    Model *m;
    std::auto_ptr<Model> t(new Model(path));
    std::pair<ModelSet::iterator, bool> r = modelFiles.insert(t.get());
    if (r.second) {
        m = t.release();
        m->registerResource();
    } else
        m = *r.first;
    return Ref(m);
}

void Model::draw(const Color tcolor, const Color lcolor) const
{
    if (!vcount_) {
        if (this != &kCube)
            kCube.draw(tcolor, lcolor);
        return;
    }

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_SHORT, 0, vdata_);
    if (tcount_) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glColor3ubv(tcolor.c);
        glDrawElements(GL_TRIANGLES, tcount_ * 3, ttype_, tdata_);
    }
    if (lcount_) {
        glColor3ubv(lcolor.c);
        glDrawElements(GL_LINES, lcount_ * 2, ltype_, ldata_);
    }

    glPopClientAttrib();
    glPopAttrib();
}

std::string Model::name() const
{
    return path_;
}

void Model::loadResource()
{
    setLoaded(true);
}

void Model::unloadResource()
{
    if (!data_)
        return;
    free(data_);
    data_ = 0;
    setLoaded(false);
}

Model::Model(std::string const &path)
    : path_(path), data_(0), datalen_(0), scale_(1.0),
      vtype_(0), vcount_(0), vdata_(0),
      ttype_(0), tcount_(0), tdata_(0),
      ltype_(0), lcount_(0), ldata_(0)
{ }

Model::~Model()
{
    free(data_);
}
