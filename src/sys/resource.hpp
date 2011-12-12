#ifndef SYS_RESOURCE_HPP
#define SYS_RESOURCE_HPP
#include <string>

class Resource {
public:
    static void loadAll();
    // Mark all "graphics resources" as unloaded.
    static void resetGraphics();

    virtual std::string name() const = 0;

    unsigned int refcount() const { return refcount_; }
    bool loaded() const { return loaded_; }
    void setLoaded(bool v) { loaded_ = v; }
    void incref() { refcount_++; }
    void decref() { refcount_--; }

protected:
    Resource(bool graphicsResource)
        : loaded_(false), registered_(false), graphicsResource_(graphicsResource),
          refcount_(0)
    { }
    virtual ~Resource();

    void registerResource();
    virtual void loadResource() = 0;
    virtual void unloadResource() = 0;
    virtual void markUnloaded(); // "graphics resources" overload this

private:
    Resource(Resource const &);
    Resource &operator=(Resource const &);

    bool loaded_, registered_, graphicsResource_;
    unsigned int refcount_;
};

#endif
