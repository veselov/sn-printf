
#ifndef VPS_SN_PRINTF_
#define VPS_SN_PRINTF_

#include "sn-printf-config.h"

#ifndef SYM_WRAP
#define SYM_WRAP(a) a
#endif

typedef struct sn_printf_ops {
    void* (*memchr)(void const * s, int c, size_t n);
    void* (*memcpy)(void * d, void const * s, size_t n);
    size_t (*strlen)(const char *c);
    void * (*memmove)(void * d, const void * s, size_t n);
    void * (*memset)(void * d, int c, size_t n);
    char * (*strchr)(char const * s, int c);
} sn_printf_ops_t;

#define sn_printf_v SYM_WRAP(sn_printf_v)
#define sn_printf SYM_WRAP(sn_printf)
#define sn_ops SYM_WRAP(sn_printf_ops)

extern int sn_printf_v(char * out, size_t lim, char const * fmt, va_list ap);
extern int sn_printf(char * out, size_t lim, char const * fmt, ...);
extern sn_printf_ops_t sn_printf_ops;

#ifdef SN_PRINTF_TEST
extern int abort_on_null_str;
#endif

#endif
