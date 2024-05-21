
#include "sn-printf.h"

enum mode {
    BARE,
    FLAGS,
    WIDTH,
    PRECISION_DOT,
    PRECISION_FIRST,
    PRECISION,
    MODIFIER,
    CONVERSION
};

struct ctx {

    char * out;
    size_t lim;
    int res;
    char const * fmt;

    char const * o_fmt;

};

struct info {

    enum mode mode;
    int width;
    int precision;
    char mod;
    union {
        int64_t i64;
        uint64_t u64;
    };
    char conversion;
    int bytes;
    int is_signed : 1;
    int hash : 1;
    int zero : 1;
    int minus : 1;
    int space : 1;
    int plus : 1;
    int sep : 1;
    int I : 1;
    int finished : 1;
    int has_precision : 1;
    int mod2 : 1; // double modifier, i.e., for hh or ll

};

sn_printf_ops_t sn_printf_ops;
static sn_printf_ops_t * ops = &sn_printf_ops;

#if WITH_UNIT_TEST
int abort_on_test = 1;
int abort_on_null_str = 1;
#endif

// Some rules:
// 1. If we run into something we don't understand, we print out the error into the output
//    and stop, to avoid any damage from misinterpreting further positional parameters
// 2. We are as much as possible compliant to C11 printf
// 3. Positional parameters are not supported
// 4. Thousands separator are always in en_US locale (commas)

static inline void write_data(struct ctx * ctx, char const * data, size_t len) {

    ctx->res += len;
    if (ctx->lim < len) {
        if (!(len = ctx->lim)) {
            return;
        }
    }
    ops->memcpy(ctx->out, data, len);
    ctx->out += len;
    ctx->lim -= len;

}

static inline void write_data_z(struct ctx * ctx, char const * data) {

    write_data(ctx, data, ops->strlen(data));

}

static int error(struct ctx * ctx, char const * err) {

    char e_buf[1024];
    sn_printf(e_buf, 1023, "<zoinks> %s, format position %d", err, ctx->fmt - ctx->o_fmt);
    e_buf[1023] = 0;

    write_data_z(ctx, e_buf);

#if WITH_UNIT_TEST
    if (abort_on_test) {
        fprintf(stderr, "%s\n", e_buf);
        abort();
    }
#endif

    // make it suitable for returning directly
    return ctx->res - 1;

}

static int int_convert(struct ctx * ctx, struct info * info) {

    char buf[256];
    size_t buf_lim = 256;

    char * ptr = buf + buf_lim;
    int len = 0;
    int sign;
    int is_zero;
    int reserve = 0;
    uint64_t val;
    if (info->is_signed) {
        int64_t s = info->i64;
        if (s < 0) {
            sign = 1;
            s = -s;
        } else {
            sign = 0;
        }
        val = s;
        if (sign || info->plus || info->space) {
            // a sign eats one unit of width, if present
            reserve++;
        }
    } else {
        val = info->u64;
    }
    is_zero = val == 0;

#define WRITE(c) do { \
    if (ptr <= buf) { error(ctx, "overflow"); return 1; } \
    *(--ptr) = (char)c;          \
    len++;                  \
} while(0)

#define WRITE_SIGN() do { \
    if (info->is_signed) {      \
        if (sign) {       \
            WRITE('-');              \
        } else if (info->plus) { \
            WRITE('+');              \
        } else if (info->space) {\
            WRITE(' ');              \
        }\
    }                      \
    \
} while(0)

#define WRITE_ALTER() do { \
    if (info->hash) {      \
        if (div == 8 && !is_zero) {       \
            WRITE('0');              \
        } else if (div == 16) { \
            WRITE(info->conversion);              \
            WRITE('0');               \
        }\
    } \
    \
} while(0)

    if (!info->has_precision) {
        info->precision = 1;
    }

    // Why can't I shift if I'm shifting the entire value?
    // I don't know, but (~0)<<64 is -1, and not 0.
    if (info->bytes < sizeof(val)) {
        val &= ~((~0llu) << (info->bytes * 8));
    }

    int div;
    int hex;
    int sep = 0;
    if (info->conversion == 'o') {
        div = 8;
        if (info->hash && val) {
            reserve++;
        }
    } else if (info->conversion == 'x') {
        hex = 'a' - 10;
        div = 16;
        if (info->hash) {
            reserve+=2;
        }
    } else if (info->conversion == 'X') {
        hex = 'A' - 10;
        div = 16;
        if (info->hash) {
            reserve+=2;
        }
    } else {
        div = 10;
        sep = info->sep;
    }

    int next_sep = 0;
    int sep_count;
    if (sep) { sep_count = 3; }

    while (val != 0) {

        if (next_sep) {
            WRITE(',');
            next_sep = 0;
            // increase the precision, because we compare it later with
            // length of output, but we wrote a non-digit char.
            info->precision++;
        }

        if (sep && --sep_count == 0) {
            sep_count = 3;
            next_sep = 1;
        }

        int one = val % div;
        val /= div;
        if (one >= 10) {
            one += hex;
        } else {
            one += '0';
        }
        WRITE(one);

    }

    // until we reached precision, fill with zeroes
    while (len < info->precision) {
        WRITE('0');
    }

    // right alignment is somewhat special, handle it first.
    if (info->minus) {
        // write out any prefixes.
        WRITE_SIGN();
        WRITE_ALTER();

        int room = info->width - len;
        if (room > 0) {

            ops->memmove(ptr - room, ptr, len);
            ptr -= room;
            ops->memset(ptr + len, ' ', room);
            len += room;

        }

    } else {

        // zeroes have to come after prefixes
        // but blanks go before prefixes
        // ugh.

        if (info->zero) {
            while (len < (info->width-reserve)) {
                WRITE('0');
            }
            WRITE_SIGN();
            WRITE_ALTER();
        } else {

            WRITE_SIGN();
            WRITE_ALTER();
            while (len < info->width) {
                WRITE(' ');
            }

        }

    }

    write_data(ctx, ptr, len);

    return 0;

#undef WRITE
#undef WRITE_SIGN
}

static inline size_t get_s_len(struct info * info, char const * s) {

    if (!info->has_precision) {
        return ops->strlen(s);
    }

    if (info->precision <= 0) { return 0; }

    char * z = ops->memchr(s, 0, info->precision);
    if (!z) { return info->precision; }
    return z - s;

}

static void pad_string(struct ctx * ctx, struct info * info, size_t s_len) {

    int pad = info->width - (int)s_len;
    while (pad > 0) {
        int f_len = pad > 2048 ? 2048 : pad;
        char n[f_len];
        ops->memset(n, ' ', f_len);
        write_data(ctx, n, f_len);
        pad -= f_len;
    }

}

int sn_printf_v(char * out, size_t lim, char const * fmt_arg, va_list ap) {

    struct ctx ctx = {
            .out = out,
            .lim = lim,
            .o_fmt = fmt_arg,
            .fmt = fmt_arg,
    };

    char const * e_fmt = ctx.fmt + ops->strlen(ctx.fmt);
    while (ctx.fmt != e_fmt) {

        char const * pct = ops->strchr(ctx.fmt, '%');
        if (!pct) {
            write_data(&ctx, ctx.fmt, e_fmt - ctx.fmt);
            break;
        }

        write_data(&ctx, ctx.fmt, pct-ctx.fmt);
        ctx.fmt = pct + 1;

        // ok, we are at '%', let's figure out what we need to dump out.

        struct info info = {.mode=BARE };

        while (ctx.fmt != e_fmt) {

            unsigned char c = *(ctx.fmt++);
            if (info.mode == BARE) {
                if (c == '%') {
                    info.finished = -1;
                    write_data(&ctx, "%", 1);
                    break;
                }
            }

#define FLAG(chr, field) if (c == chr) { \
   info.field = -1;              \
   info.mode = FLAGS; /* reset BARE */                              \
   continue;                              \
} do {} while(0)

            if (info.mode == BARE || info.mode == FLAGS) {
                FLAG('#', hash);
                FLAG('0', zero);
                FLAG('-', minus);
                FLAG(' ', space);
                FLAG('+', plus);
                FLAG('\'', sep);
                FLAG('I', I);
                // we couldn't find any flags
                info.mode = WIDTH;
            }

            if (info.mode == WIDTH) {

                if (c == '*' && info.width == 0) {
                    // man says - width must be int
                    info.width = va_arg(ap, int);
                    info.mode = PRECISION_DOT;
                    continue;
                }

                // otherwise, it must be a number
                if (c >= '0' && c <= '9') {
                    info.width *= 10;
                    info.width += (c - '0');
                    continue;
                }

                // end of width, continue to precision
                info.mode = PRECISION_DOT;

            }

            if (info.mode == PRECISION_DOT) {
                if (c == '.') {
                    info.mode = PRECISION_FIRST;
                    info.has_precision = -1;
                    continue;
                }
                info.mode = MODIFIER;
            }

            if (info.mode == PRECISION_FIRST) {
                if (c == '-') {
                    info.mode = PRECISION;
                    info.has_precision = 0;
                    continue;
                }
                if (c == '*') {
                    info.mode = MODIFIER;
                    // man says - precision must be of type int.
                    info.precision = va_arg(ap, int);
                    continue;
                }
                if (c >= '0' && c <= '9') {
                    info.precision = (c - '0');
                    info.mode = PRECISION;
                    continue;
                }
                // not a precision character, move on
                info.mode = MODIFIER;
            }

            if (info.mode == PRECISION) {
                if (c >= '0' && c <= '9') {
                    info.precision *= 10;
                    info.precision += (c - '0');
                    continue;
                }
                info.mode = MODIFIER;
            }

            if (info.mode == MODIFIER) {

                info.mode = CONVERSION;

                if (info.mod != 0) {
                    // we only accept double l and double h
                    // everything else is single char
                    if (c == 'h' && info.mod == 'h') {
                        info.mod2 = -1;
                        continue;
                    } else if (c == 'l' && info.mod == 'l') {
                        info.mod2 = -1;
                        continue;
                    }
                } else {

                    switch (c) {
                        case 'h':
                        case 'l':
                            info.mode = MODIFIER; // back to modifier for these two
                        case 'L':
                        case 'j':
                        case 'z':
                        case 't':
                            info.mod = c;
                            continue;
                    }

                }
            }

            if (info.has_precision && info.precision < 0) {
                info.has_precision = 0;
            }

            // don't allow exuberant widths in any case.
            if (info.width >= 256) {
                return error(&ctx, "Width too large");
            }

            // at this point, the char must be a conversion that we understand, or it's an error.

#define PICK2(_is_signed, pick_type, unsigned_pick_type, width_type) do { \
    info.is_signed = (_is_signed) ? -1 : 0; \
    if (info.is_signed) {                           \
        info.i64 = va_arg(ap, pick_type);               \
    } else {                                        \
        info.u64 = va_arg(ap, unsigned_pick_type); \
    } \
    info.bytes = sizeof(width_type); \
} while (0)

#define PICK(_is_signed, pick_type, width_type) PICK2(_is_signed, pick_type, unsigned pick_type, width_type)

#define PICK0(pick_type) PICK2(0, pick_type, pick_type, pick_type)


            info.conversion = c;
            // we can declare "finished" here, we'll either
            // pick a conversion character we recognize, or declare an error.
            info.finished = -1;

            if (c == 'd' || c == 'i' || c == 'x' || c == 'X' || c == 'o' || c == 'u') {

                int is_signed = c == 'd' || c == 'i';
                if (info.mod == 'h') {
                    if (info.mod2) {
                        PICK(is_signed, int, char);
                    } else {
                        PICK(is_signed, int, char);
                    }
                } else if (info.mod == 'l') {
                    if (info.mod2) {
                        PICK(is_signed, long long, long long);
                    } else {
                        PICK(is_signed, long, long);
                    }
                } else if (info.mod == 'j') {
                    PICK2(is_signed, intmax_t, uintmax_t, intmax_t);
                } else if (info.mod == 'z') {
                    PICK2(is_signed, size_t, ssize_t, size_t);
                } else if (info.mod == 't') {
                    PICK0(ptrdiff_t);
                } else {
                    PICK(is_signed, int, int);
                }

                if (int_convert(&ctx, &info)) {
                    return ctx.res - 1;
                }
                break;

            }

            if (c == 's') {

                char const * s = va_arg(ap, char const*);

                if (info.mod == 'l') {
                    return error(&ctx, "wchar_t not supported");
                }

                if (!s) {
#if WITH_UNIT_TEST
                    if (abort_on_null_str) {
                        abort();
                    }
#endif
                    s = "(null)"; // the Linux way
                }

                size_t s_len = get_s_len(&info, s);
                if (!info.minus) {
                    pad_string(&ctx, &info, s_len);
                }
                write_data(&ctx, s, s_len);
                if (info.minus) {
                    pad_string(&ctx, &info, s_len);
                }

                break;

            }

            if (c == 'c') {
                if (info.mod == 'l') {
                    return error(&ctx, "wint_t is not supported");
                }
                // TODO: OK, C is weird.
                // PRINTF(3) says that %c is retrieved as an int, converted to
                // unsigned char, and dumped into the output. But the output is
                // signed char[], so what gives?
                char ch = (unsigned char) va_arg(ap, int);
                write_data(&ctx, &ch, 1);
                break;
            }

            if (c == 'p') {
                info.hash = -1;
                info.conversion = 'x';
                info.u64 = (uint64_t) (uintptr_t) va_arg(ap, void*);
                info.bytes = sizeof(void*);
                if (int_convert(&ctx, &info)) {
                    return ctx.res-1;
                }
                break;
            }

            return error(&ctx, "Unknown conversion");

        }

        if (!info.finished) {
            return error(&ctx, "Incomplete sequence");
        }

    };

    write_data(&ctx, "", 1);

    // return value is the number of characters (excluding the
    // terminating null byte) which would have been written to the final string if enough
    // space had been available.
    return ctx.res - 1;

#undef WRITE

}

int sn_printf(char * out, size_t lim, char const * fmt, ...) {

    va_list ap;
    va_start(ap, fmt);
    int r = sn_printf_v(out, lim, fmt, ap);
    va_end(ap);
    return r;

}
