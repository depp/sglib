#include "resource.hpp"
#include <vector>

static std::vector<Resource *> resources;

void Resource::loadAll()
{
    std::vector<Resource *>::iterator
        b = resources.begin(), i = b, e = resources.end();
    try {
        while (i != e) {
            Resource *r = *i;
            if (r->refcount_ > 0) {
                if (!r->loaded_) {
                    r->loadResource();
                    r->loaded_ = true;
                }
                ++i;
            } else {
                r->unloadResource();
                delete r;
                --e;
                if (i != e)
                    *i = *e;
            }
        }
    } catch (std::exception const &) {
        resources.resize(e - b);
        throw;
    }
    resources.resize(e - b);
}

void Resource::resetGraphics()
{
    std::vector<Resource *>::iterator
        i = resources.begin(), e = resources.end();
    for (; i != e; ++i) {
        Resource *r = *i;
        if (r->graphicsResource_)
            r->markUnloaded();
    }
}

Resource::~Resource()
{ }

void Resource::registerResource()
{
    if (registered_)
        return;
    resources.push_back(this);
    registered_ = true;
}

void Resource::markUnloaded()
{ }
