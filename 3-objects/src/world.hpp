#ifndef WORLD_HPP
#define WORLD_HPP
class Object;

class World {
public:
    World();
    ~World();

    void addObject(Object *obj);
    void draw();
    void update();

private:
    World(const World &);
    World &operator=(const World &);

    Object *first_;
};

#endif
