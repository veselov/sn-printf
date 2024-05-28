
#include "../sn-printf.h"
#include "test-fw.h"

#include <string.h>
#include <stdio.h>

static int large_string(void);

#define SIMPLE(a,b,c...) do { \
    char * r = f_asprintf(b, ## c); \
    TEST_CHR_EQUAL(a, r); \
    free(r); \
} while(0); BOLT_NEST()

static int large_string(void) {

    int err = E_OK;

    do {

        char x[1800];
        pf_random(x, 1800);
        for (int i=0; i<1800; i++) {
            if (x[i] < ' ' || x [i] > 0x7f) {
                x[i] = 'A';
            }
        }

        x[1799] = 0;

        SIMPLE(x, "%s", x);
        char * x2 = f_strdup(x);
        x2[1600] = 0;
        SIMPLE(x2, "%.*s", 1600, x);

        free(x2);

    } while (0);

    return err;

}

int sn_printf_test(void) {

    int err = E_OK;

    do {

        BOLT_SUB(large_string());

        SIMPLE("1 8 1", "%d %" PRId64 " %d", 1, (int64_t)8, 1);
        SIMPLE("1 8 1", "%d %" PRIu64 " %d", 1, (uint64_t)8, 1);
        SIMPLE("1 8 1", "%d %" PRId32 " %d", 1, (int32_t)8, 1);
        SIMPLE("1 8 1", "%d %" PRId32 " %d", 1, (uint32_t)8, 1);

        SIMPLE("me is at 0x123e, % out", "me is at %p, %% out", 0x123e);

        SIMPLE("left: >left <, right: >  right<", "left: >%-5.4s<, right: >%7.6s<", "lefty", "right");
        SIMPLE("good:%s", "good:%s", "%s");

        SIMPLE("299,988", "%'d", 299988);
        SIMPLE("1,299,988", "%'d", 1299988);
        SIMPLE("1babe20014251-81", "%x%o%d%u%i", 0x1babe, 02001, 42, 51, -81);
        SIMPLE("-004  ", "%-*.*d", 6, 3, -4);
        SIMPLE("+004  ", "%+-*.*d", 6, 3, 4);
        SIMPLE(" 004  ", "% -*.*d", 6, 3, 4);
        SIMPLE("0", "%#*o", -100, 0);
        SIMPLE("0177", "%#*o", -100, 0177);
        SIMPLE("0X1FAD", "%#X", 0x1fad);
        SIMPLE("-004  ", "%-6.3d", -4);
        SIMPLE("  -004", "%6.3d", -4);
        SIMPLE("-00004", "%06.3d", -4);
        SIMPLE("0xdea1d", "%#x", 0xdea1d);
        SIMPLE("before 2490 and after 7631213", "before %d and after %d", 2490, 7631213);
        SIMPLE("0", "%d", 0);
        SIMPLE("", "%.0d", 0);
        SIMPLE("", "%.d", 0);
        SIMPLE("just format", "just format");

    } while (0);

    return err;

}

#undef SIMPLE


int main(int argc, char ** argv) {

    int err = E_OK;

    do {
        BOLT_SUB(sn_printf_test());
        sn_printf_ops.strchr = strchr;
        sn_printf_ops.memset = memset;
        sn_printf_ops.memmove = memmove;
        sn_printf_ops.memcpy = memcpy;
        sn_printf_ops.strlen = strlen;
        sn_printf_ops.memchr = memchr;
        BOLT_SUB(sn_printf_test());
    } while (0);

    printf("test result: %d\n", err);

    return err;

}