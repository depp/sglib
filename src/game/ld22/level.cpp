#include "level.hpp"
#include "walker.hpp"
#include <cstring>
#include <memory>
#include "sys/path.hpp"
#include "sys/ifile.hpp"
using namespace LD22;

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
    playerx = SCREEN_WIDTH / 2 - Walker::WWIDTH / 2;
    playery = TILE_SIZE;
    background = 0;
}

enum {
    DATA_EOF,
    DATA_AREA,
    DATA_BACKGROUND,
    DATA_PLAYER
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

        case DATA_PLAYER:
            if (e - p < 4)
                goto err;
            playerx = (p[0] << 8) | p[1];
            playery = (p[2] << 8) | p[3];
            p += 4;
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

    {
        unsigned char buf[5];
        buf[0] = DATA_PLAYER;
        buf[1] = playerx >> 8;
        buf[2] = playerx;
        buf[3] = playery >> 8;
        buf[4] = playery;
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
