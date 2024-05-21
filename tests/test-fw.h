#ifndef SNP_TEST_FW_H_
#define SNP_TEST_FW_H_

#include <stdlib.h>
#include <stdarg.h>

#define E_OK 0
#define E_INTERNAL 1

#define HOW_ERR 1
#define HOW_FATAL 2
#define HOW_MSG 3

#define LINE_OUT(how, str, args...) debug_out(__func__, chop_path(__FILE__), __LINE__, how, str, ## args)
#define DBG(a,b...) do { LINE_OUT(HOW_MSG, a, ##b); } while(0)
#define BOLT_IF(cond, __err, msg, x...) if ((cond)) { err = (__err); DBG(msg ", setting err %d", ## x, err); break; } do{}while(0)
#define BOLT_NEST() BOLT_SUB(err)
#define BOLT_SUB(a) { err = (a); if (err != E_OK) { BOLT_SAY(err, #a); }} do{}while(0)
#define BOLT_SAY(__err, msg, x...) err = (__err); DBG(msg ", setting err %d", ## x, err); break; do{}while(0)
#define FATAL(a,b...) do { LINE_OUT(HOW_FATAL, a, ##b); } while(0)

void v_debug_out(char const * func, char const * file, int line, int how, char const * str, va_list va);
char * f_asprintf(const char * fmt, ...);
char * f_strdup(const char * s);
void * f_malloc(size_t t);
const char * chop_path(const char *);
void pf_random(void * to, size_t where);
void debug_out(char const * func, char const * file, int line, int how, char const * str, ...);
int z_strcmp(const char * s1, const char * s2);

#define TEST_CHR_EQUAL(chr1, chr2) BOLT_IF(z_strcmp(chr1, chr2), E_INTERNAL, "String %s was expected to be equal to %s", chr2, chr1)

#endif