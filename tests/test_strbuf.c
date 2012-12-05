/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#include "libpce/strbuf.h"

int main(int argc, char *argv[])
{
    struct pce_strbuf ba, bb, bc;
    char *p, s[2];
    int i;

    pce_strbuf_init(&ba, 0);
    pce_strbuf_init(&bb, 100);
    pce_strbuf_init(&bc, 0);

    p = bb.s;
    for (i = 0; i < 100; i++) {
        pce_strbuf_putc(&ba, i + 101);
        pce_strbuf_putc(&bb, i + 1);
    }
    assert(bb.s == p);
    for (i = 0; i < 100; i++) {
        assert(ba.s[i] == (char) (i + 101));
        assert(bb.s[i] == (char) (i + 1));
    }
    assert(pce_strbuf_len(&ba) == 100);
    assert(pce_strbuf_len(&bb) == 100);
    assert(strlen(ba.s) == 100);
    assert(strlen(bb.s) == 100);

    p = ba.s;
    pce_strbuf_clear(&ba);
    for (i = 0; i < 100; i++) {
        s[0] = i + 2;
        s[1] = '\0';
        pce_strbuf_puts(&ba, s);
    }

    assert(ba.s == p);
    for (i = 0; i < 100; i++) {
        assert(ba.s[i] == (char) (i + 2));
    }

    p = pce_strbuf_detach(&bb);

    pce_strbuf_destroy(&ba);
    pce_strbuf_destroy(&bb);
    free(p);

    p = pce_strbuf_detach(&bc);
    assert(strlen(p) == 0);
    free(p);
    pce_strbuf_destroy(&bc);

    return 0;
}
