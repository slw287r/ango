#define _GNU_SOURCE
#include <inttypes.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <zlib.h>
#include "kstring.h"
#include "khashl.h"
#include "ketopt.h"

#define VERSION "0.1.0"
extern const char *__progname;

#define error(msg, ...) do {                                 \
    fputs("\e[31m\xE2\x9C\x98\e[0m ", stderr);               \
    fprintf(stderr, msg, ##__VA_ARGS__);                     \
    if (errno) fprintf(stderr, ": %s\n", strerror(errno));   \
    fflush(stderr);                                          \
    exit(EXIT_FAILURE);                                      \
} while (0)

typedef struct {
	char *in, *out, *go, *egg;
} arg_t;

ko_longopt_t long_options[] = {
	{ "in",           ko_required_argument, 'i' },
	{ "out",          ko_required_argument, 'o' },
	{ "go",           ko_required_argument, 'g' },
	{ "egg",          ko_required_argument, 'e' },
	{ "help",         ko_no_argument, 'h' },
	{ "version",      ko_no_argument, 'v' },
	{ NULL, 0, 0 }
};

// go basic
typedef struct
{
	char *name;
	char *namespace;
} gb_t;

// go id to name/space map
KHASHL_MAP_INIT(KH_LOCAL, go_t, go, uint32_t, gb_t *, kh_hash_uint32, kh_eq_generic)

// seq to go map
KHASHL_CMAP_INIT(KH_LOCAL, sg_t, sg, kh_cstr_t, uint32_t *, kh_hash_str, kh_eq_str)

char *kzgets(char *buf, int siz, gzFile fp) { return gzgets(fp, buf, siz); }
void let_go(const char *fn, go_t *h);
void eat_egg(const char *fn, sg_t *h);
void map_go(const char *in, const go_t *g, const sg_t *s, const char *out);
void chkfile(int num, ...);
void usage();
