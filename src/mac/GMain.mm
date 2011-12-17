#import "sys/rand.hpp"
#import "sys/path_posix.hpp"
#import "sys/clock.hpp"
#import <getopt.h>

bool gEditor;

static void init(int argc, char *argv[])
{
    int opt;
    const char *altpath = NULL;

    while ((opt = getopt(argc, argv, "ei:")) != -1) {
        switch (opt) {
        case 'e':
            gEditor = true;
            break;
        
        case 'i':
            altpath = optarg;
            break;
        
        default:
            fputs("Invalid usage\n", stderr);
            exit(1);
        }
    }
    
    pathInit(altpath);
    initTime();
    Rand::global.seed();
}

int main(int argc, const char *argv[])
{
    init(argc, (char **) argv);
    return NSApplicationMain(argc, argv);
}
