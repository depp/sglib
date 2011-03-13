#ifndef SYS_RESOURCE_HPP
#define SYS_RESOURCE_HPP
#include <string>

class Resource {
public:
    static void loadAll();

    virtual std::string name() const = 0;

    unsigned int refcount() const { return refcount_; }
    bool loaded() const { return loaded_; }
    void setLoaded(bool v) { loaded_ = v; }
    void incref() { refcount_++; }
    void decref() { refcount_--; }

protected:
    Resource()
        : loaded_(false), registered_(false), refcount_(0)
    { }
    virtual ~Resource();

    void registerResource();
    virtual void loadResource() = 0;
    virtual void unloadResource() = 0;

private:
    Resource(Resource const &);
    Resource &operator=(Resource const &);

    bool loaded_, registered_;
    unsigned int refcount_;
};

#endif
