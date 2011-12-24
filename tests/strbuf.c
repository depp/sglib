#include "impl/strbuf.h"
#include <stdio.h>
#include <stdlib.h>

static void test(const char *a, const char *b, const char *result)
{
    struct sg_strbuf buf;
    size_t l, bl;
    sg_strbuf_init(&buf, 0);
    sg_strbuf_puts(&buf, a);
    sg_strbuf_joinstr(&buf, b);
    l = strlen(result);
    bl = sg_strbuf_len(&buf);
    if (l != bl || memcmp(buf.s, result, l + 1)) {
        printf(
            "FAILED\n"
            "  '%s' + '%s'\n"
            "  expected '%s'\n"
            "  got '%s'\n",
            a, b, result, buf.s);
        exit(1);
    }
    sg_strbuf_destroy(&buf);
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    test("aa", "../bb", "bb");
    test("aa", "bb//cc", "aa/bb/cc");
    test("aa", "bb/", "aa/bb");
    test("aa", ".", "aa");
    test("aa", "/bb", "/bb");
    test("aa", "../..", "..");
    test("aa", "./bb/cc//../ee///./ff///", "aa/bb/ee/ff");
    test("aa/bb/cc", "../../dd", "aa/dd");
    test("/aa/bb", "../..", "/");
    test("/usr/local", "../../..", "/");
    test("aa", "..", ".");
    test(".", "aa", "aa");
    test("", "aa", "aa");
    test("aa", "", "aa");
    test("", "", ".");
    return 0;
}
