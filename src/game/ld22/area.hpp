#include <vector>
namespace LD22 {
class Actor;

/* One "screen" of level area.  */
class Area {
public:
    static const int WIDTH = 24, HEIGHT = 15;

private:
    unsigned char m_tile[HEIGHT][WIDTH];
    std::vector<Actor *> m_actors;

public:
    Area();
    ~Area();

    int getTile(int x, int y) const
    {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
            return m_tile[y][x];
        return 0;
    }

    void setTile(int x, int y, int v)
    {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
            m_tile[y][x] = v;
    }

    void addActor(Actor *a);

    void draw();
};

}
