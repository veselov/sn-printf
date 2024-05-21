
#include "test-fw.h"
#include "../sn-printf.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void debug_out(char const * func, char const * file, int line, int how, char const * str, ...) {
    va_list ap;
    va_start(ap, str);
    v_debug_out(func, file, line, how, str, ap);
    va_end(ap);
}

void * f_malloc(size_t t) {

    void * r = calloc(1, t);
    if (!r) {
        FATAL("Failed to malloc %ld bytes", t);
    }

    return r;

}


char * f_strdup(const char * s) {
    if (!s) { return 0; }
    size_t l = strlen(s) + 1;
    char * r = f_malloc(l);
    return memcpy(r, s, l);
}

const char * chop_path(const char * path) {
    const char * aux = strrchr(path, '/');
    if (aux) { return aux + 1; }
    return path;
}

int z_strcmp(const char * s1, const char * s2) {

    if (!s1) {
        if (!s2) { return 0; }
        return -1;
    }

    if (!s2) { return 1; }

    return strcmp(s1, s2);

}

void v_debug_out(char const * func, char const * file, int line, int how, char const * str, va_list va) {

    char const * eol;

#if XL4BUS_ANDROID || WITH_UNIT_TEST
    eol = "";
#else
    eol = "\n";
#endif

    char const * how_str = "";
#if !XL4BUS_ANDROID
    if (how == HOW_ERR) {
        how_str = "ERR ";
    }
#endif

    // time func:file:line how <orig>
    char * final_fmt = f_asprintf("%s:%s:%d %s%s%s", func, file, line, how_str, str, eol);

    vfprintf(stderr, final_fmt, va);
    if (how == HOW_FATAL) {
        abort();
    }

    free(final_fmt);

}

char * f_asprintf(const char * fmt, ...) {

    va_list ap;

    va_start(ap, fmt);

    int rc = sn_printf_v(0, 0, fmt, ap);
    va_end(ap);

    va_start(ap, fmt);

    char * ret = malloc(rc + 1);
    if (ret) {
        sn_printf_v(ret, rc + 1, fmt, ap);
    }

    va_end(ap);

    return ret;

#undef PRINTF_BUF


}

void pf_random(void * to, size_t where) {

    do {

        int fd = open("/dev/urandom", O_RDONLY);
        if (fd < 0) { break; }
        while (where) {
            ssize_t rc = read(fd, to, where);
            if (rc < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ERESTART) {
                    continue;
                }
                perror("read /dev/random");
                break;
            } else if (rc == 0) {
                // EOF before where is expended?
                break;
            }
            // rc is >0 here
            where -= rc;
            to += rc;
        }

        close(fd);

        if (!where) { return; }

    } while (0);

    abort();

}
