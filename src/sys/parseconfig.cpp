#include "configfile.hpp"
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fputs("Usage: parseconfig IN OUT\n", stderr);
        return 1;
    }
    ConfigFile f;
    f.read(argv[1]);
    f.write(argv[2]);
    return 0;
}
