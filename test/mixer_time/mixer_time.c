/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/mixer/time.h"

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
    int samplerate, bufsize, pos, amt, offset, outtime;
    double input[2], intime, output[2];

    if (argc != 8) {
        fputs("Usage: time_test RATE BUFSIZE DELTATIME MIXAHEAD "
              "SAFETY MAXSLOPE SMOOTHING\n",
              stderr);
        return 1;
    }

    samplerate = (int) strtol(argv[1], NULL, 0);
    bufsize = (int) strtol(argv[2], NULL, 0);
    sg_mixer_time_init(&time, samplerate, bufsize);
    parse_double(&time.deltatime, argv[3]);
    parse_double(&time.mixahead, argv[4]);
    parse_double(&time.safety, argv[5]);
    parse_double(&time.maxslope, argv[6]);
    parse_double(&time.smoothing, argv[7]);

    offset = 0;
    intime = 0;
    while (1) {
        pos = 0;
        while (pos < (int) sizeof(input)) {
            amt = fread((char *) &input + pos,
                        1, sizeof(input) - pos,
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
            output[0] = intime * 0.01;
            outtime = sg_mixer_time_get(&time, intime * 0.01);
            if (outtime >= bufsize)
                break;
            output[1] = outtime + offset;
            amt = fwrite(&output, 1, sizeof(output), stdout);
            if (amt != sizeof(output))
                abort();
            intime++;
        }
        offset += bufsize;
    }

    return 0;
}
