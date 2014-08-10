#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/mixer/time.h"

static void
parse_int(int *x, const char *s)
{
    if (!strcmp(s, "-"))
        return;
    *x = (int) strtol(s, NULL, 0);
}

static void
parse_double(double *x, const char *s)
{
    if (!strcmp(s, "-"))
        return;
    *x = strtod(s, NULL);
}

int
main(int argc, char **argv)
{
    struct sg_mixer_time time;
    int bufsize, pos, amt, offset, outtime;
    unsigned input[2], intime;

    if (argc != 7) {
        fputs("Usage: time_test BUFSIZE DELTABITS MIXAHEAD "
              "SAFETY MAXSLOPE SMOOTHING\n",
              stderr);
        return 1;
    }

    bufsize = (int) strtol(argv[1], NULL, 0);
    sg_mixer_time_init(&time, bufsize);
    parse_int(&time.deltabits, argv[2]);
    parse_int(&time.mixahead, argv[3]);
    parse_double(&time.safety, argv[4]);
    parse_double(&time.maxslope, argv[5]);
    parse_double(&time.smoothing, argv[6]);

    offset = 0;
    intime = 0;
    while (1) {
        pos = 0;
        while (pos < (int) (sizeof(unsigned) * 2)) {
            amt = fread((char *) &input + pos,
                        1, sizeof(unsigned) * 2 - pos,
                        stdin);
            if (!amt) {
                if (feof(stdin) && pos == 0)
                    return 0;
                abort();
            }
            pos += amt;
        }

        sg_mixer_time_update(&time, input[0], input[1]);
        while (1) {
            outtime = sg_mixer_time_get(&time, intime);
            if (outtime >= bufsize)
                break;
            outtime += offset;
            amt = fwrite(&outtime, 1, sizeof(int), stdout);
            if (amt != sizeof(int))
                abort();
            intime++;
        }
        offset += bufsize;
    }

    return 0;
}
