#include "level.hpp"
#include "walker.hpp"
#include <cstring>
#include <memory>
#include "sys/path.hpp"
#include "sys/ifile.hpp"
using namespace LD22;

const char *Entity::typeName(Type t)
{
    switch (t) {
    case Null: return "null";
    case Player: return "Player";
    default: return "<unknown>";
    }
}

static const int TILE_COUNT = TILE_WIDTH * TILE_HEIGHT;

std::string Level::pathForLevel(int num)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "level/%02d.dat", num);
    return std::string(buf);
}

void Level::clear()
{
    std::memset(tiles, 0, TILE_COUNT);
    std::memset(tiles, 1, TILE_WIDTH);
    background = 0;
    entity.clear();
}

enum {
    DATA_EOF,
    DATA_AREA,
    DATA_BACKGROUND,
    DATA_ENTITY
};

void Level::load(int num)
{
    std::string path = pathForLevel(num);
    std::auto_ptr<IFile> f(Path::openIFile(path));
    Buffer b = f->readall();
    f.reset();
    const unsigned char *p = b.getUC(), *e = p + b.size();

    clear();

    while (1) {
        if (p == e)
            goto err;
        switch (*p++) {
        case DATA_EOF:
            return;

        case DATA_AREA:
            if (e - p < TILE_COUNT)
                goto err;
            std::memcpy(tiles, p, TILE_COUNT);
            p += TILE_COUNT;
            break;

        case DATA_BACKGROUND:
            if (e == p)
                goto err;
            background = *p++;
            break;

        case DATA_ENTITY:
            if (e == p)
                goto err;
            {
                int num = *p;
                p++;
                if (e - p < 5 * num)
                    goto err;
                entity.resize(num);
                for (int i = 0; i < num; ++i) {
                    Entity &e = entity[i];
                    if (*p > Entity::MAX_TYPE)
                        goto err;
                    e.type = (Entity::Type) *p;
                    e.x = (p[1] << 8) | p[2];
                    e.y = (p[3] << 8) | p[4];
                    p += 5;
                }
            }
            break;

        default:
            goto err;
        }
    }

err:
    fputs("invalid level data\n", stderr);
    abort();
}

void Level::save(int num)
{
    std::string path = pathForLevel(num);
    fprintf(stderr, "saving %s...\n", path.c_str());
    FILE *f = Path::openOFile(path);

    fputc(DATA_AREA, f);
    fwrite(tiles, TILE_COUNT, 1, f);

    fputc(DATA_BACKGROUND, f);
    fputc(background, f);

    fputc(DATA_ENTITY, f);
    fputc(entity.size(), f);
    std::vector<Entity>::const_iterator
        i = entity.begin(), e = entity.end();
    for (; i != e; ++i) {
        unsigned char buf[5];
        buf[0] = (int) i->type;
        buf[1] = i->x >> 8;
        buf[2] = i->x;
        buf[3] = i->y >> 8;
        buf[4] = i->y;
        fwrite(buf, 5, 1, f);
    }

    fputc(DATA_EOF, f);

    if (ferror(f)) {
        fputs("Level::save failed\n", stderr);
        abort();
    }
    fclose(f);
    fputs("save successful\n", stderr);
}
