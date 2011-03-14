#include "model.hpp"
#include "color.hpp"
#include "sys/path.hpp"
#include "sys/ifile.hpp"
#include <set>
#include <vector>
#include <memory>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    glPushMatrix();
    glScaled(scale_, scale_, scale_);

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

    glPopMatrix();
    glPopClientAttrib();
    glPopAttrib();
}

std::string Model::name() const
{
    return path_;
}

static unsigned char const MODEL_HDR[16] = "Egg3D Model\0\4\3\2\1";

struct ModelChunk {
    unsigned char name[4];
    uint32_t length;
    void *data;
};

std::vector<ModelChunk> readChunks(void *ptr, size_t len)
{
    unsigned char *data = reinterpret_cast<unsigned char *>(ptr);
    size_t pos = 0;
    std::vector<ModelChunk> chunks;
    while (1) {
        if (len - pos < 4)
            break;
        if (!memcmp(data + pos, "END ", 4))
            return chunks;
        if (len - pos < 8)
            break;
        chunks.resize(chunks.size() + 1);
        ModelChunk &c = chunks.back();
        memcpy(&c.name, data + pos, 4);
        memcpy(&c.length, data + pos + 4, 4);
        pos += 8;
        c.data = data + pos;
        if (len - pos < c.length)
            break;
        pos = (pos + c.length + 3) & ~3;
    }
    chunks.clear();
    return chunks;
}

void Model::loadResource()
{
    try {
        if (data_) {
            free(data_);
            data_ = 0;
            datalen_ = 0;
        }
        {
            unsigned char hdr[16];
            std::auto_ptr<IFile> f(Path::openIFile(path_));
            size_t amt = f->read(hdr, 16);
            if (amt < 16 || memcmp(hdr, MODEL_HDR, 16))
                goto invalid;
            Buffer b = f->readall();
            data_ = b.get();
            datalen_ = b.size();
            b.release();
        }
        std::vector<ModelChunk> chunks = readChunks(data_, datalen_);
        if (chunks.empty())
            goto invalid;
        std::vector<ModelChunk>::iterator
            i = chunks.begin(), e = chunks.end();
        for (; i != e; ++i) {
            if (!memcmp(i->name, "SclF", 4)) {
                if (i->length != 4)
                    goto invalid;
                scale_ = *reinterpret_cast<float *>(i->data);
            } else if (!memcmp(i->name, "VrtS", 4)) {
                if (i->length % 6)
                    goto invalid;
                vtype_ = GL_SHORT;
                vcount_ = i->length / 6;
                vdata_ = i->data;
            } else if (!memcmp(i->name, "LinS", 4)) {
                // FIXME easy to do buffer overflow
                // with indices out of range
                if (i->length % 4)
                    goto invalid;
                ltype_ = GL_UNSIGNED_SHORT;
                lcount_ = i->length / 4;
                ldata_ = i->data;
            } else if (!memcmp(i->name, "TriS", 4)) {
                if (i->length % 6)
                    goto invalid;
                ttype_ = GL_UNSIGNED_SHORT;
                tcount_ = i->length / 6;
                tdata_ = i->data;
            } else
                goto invalid;
        }
        fprintf(stderr, "Loaded model %s\n", path_.c_str());
    } catch (std::exception const &exc) {
        fprintf(stderr, "Failed to load model %s: %s\n",
                path_.c_str(), exc.what());
    }
    setLoaded(true);
    return;
invalid:
    fprintf(stderr, "Invalid model file %s\n", path_.c_str());
    setLoaded(true);
}

void Model::unloadResource()
{
    if (!data_)
        return;
    free(data_);
    data_ = 0;
    setLoaded(false);
    fprintf(stderr, "Unloaded model %s\n", path_.c_str());
}

Model::Model(std::string const &path)
    : path_(path), data_(0), datalen_(0), scale_(1.0),
      vtype_(0), vcount_(0), vdata_(0),
      ttype_(0), tcount_(0), tdata_(0),
      ltype_(0), lcount_(0), ldata_(0)
{ }

Model::~Model()
{
    ModelSet::iterator i = modelFiles.find(this);
    if (i != modelFiles.end() && *i == this)
        modelFiles.erase(i);
    free(data_);
}
