#define PROLOG_FREESTANDING

#include "bits.h"

static int pl_isspace(int c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' ||
         c == '\v';
}
static int pl_isalpha(int c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static int pl_isalnum(int c) { return pl_isalpha(c) || (c >= '0' && c <= '9'); }
static int pl_isupper(int c) { return c >= 'A' && c <= 'Z'; }

#define isspace(c) pl_isspace((unsigned char)(c))
#define isalpha(c) pl_isalpha((unsigned char)(c))
#define isalnum(c) pl_isalnum((unsigned char)(c))
#define isupper(c) pl_isupper((unsigned char)(c))

static long pl_strtol(const char *s, char **endptr, int base) {
  (void)base;
  long val = 0;
  int neg = 0;
  if (*s == '-') {
    neg = 1;
    s++;
  } else if (*s == '+') {
    s++;
  }
  while (*s >= '0' && *s <= '9')
    val = val * 10 + (*s++ - '0');
  if (endptr)
    *endptr = (char *)s;
  return neg ? -val : val;
}
#define strtol pl_strtol

static size_t pl_strcspn(const char *s, const char *reject) {
  size_t n = 0;
  while (s[n]) {
    const char *r = reject;
    while (*r) {
      if (s[n] == *r)
        return n;
      r++;
    }
    n++;
  }
  return n;
}
#define strcspn pl_strcspn

static char *pl_strncat(char *dst, const char *src, size_t n) {
  char *d = dst;
  while (*d)
    d++;
  while (n-- && (*d++ = *src++))
    ;
  *d = '\0';
  return dst;
}
#define strncat pl_strncat

#define vsnprintf(buf, size, fmt, ap) vsprintf((buf), (int)(size), (fmt), (ap))

typedef void FILE;
#define EOF (-1)
#define stderr ((FILE *)0)
#define stdin ((FILE *)0)

static int pl_fprintf(FILE *f, const char *fmt, ...) {
  (void)f;
  (void)fmt;
  return 0;
}
#define fprintf pl_fprintf

static int pl_printf(const char *fmt, ...) {
  (void)fmt;
  return 0;
}
#define printf pl_printf

static int pl_vprintf(const char *fmt, va_list ap) {
  (void)fmt;
  (void)ap;
  return 0;
}
#define vprintf pl_vprintf

static int pl_getchar(void) { return EOF; }
#define getchar pl_getchar

static FILE *pl_fopen(const char *name, const char *mode) {
  (void)name;
  (void)mode;
  return 0;
}
#define fopen pl_fopen

static int pl_fclose(FILE *f) {
  (void)f;
  return 0;
}
#define fclose pl_fclose

static char *pl_fgets(char *s, int n, FILE *f) {
  (void)s;
  (void)n;
  (void)f;
  return 0;
}
#define fgets pl_fgets

#include "prolog/src/builtins.c"
#include "prolog/src/debug.c"
#include "prolog/src/env.c"
#include "prolog/src/ffi.c"
#include "prolog/src/io.c"
#include "prolog/src/parse.c"
#include "prolog/src/print.c"
#include "prolog/src/solve.c"
#include "prolog/src/term.c"
#include "prolog/src/unify.c"

static void pl_write_str(prolog_ctx_t *ctx, const char *str, void *ud) {
  (void)ctx;
  (void)ud;
  kernel_printf("%s", str);
}

static void pl_writef(prolog_ctx_t *ctx, const char *fmt, va_list args,
                      void *ud) {
  (void)ctx;
  (void)ud;
  char buf[256];
  vsprintf(buf, (int)sizeof(buf), fmt, args);
  kernel_printf("%s", buf);
}

static void pl_write_term_hook(prolog_ctx_t *ctx, term_t *t, env_t *env,
                               void *ud) {
  (void)ud;
  print_term(ctx, t, env);
}

void prolog_demo(void) {

  prolog_ctx_t *prolog_ctx = kmalloc(sizeof(prolog_ctx_t));

  io_hooks_t hooks = {0};
  hooks.write_str = pl_write_str;
  hooks.writef = pl_writef;
  hooks.write_term = pl_write_term_hook;
  io_hooks_set(prolog_ctx, &hooks);

  parse_clause(prolog_ctx, "fact(0, 1).");
  parse_clause(prolog_ctx,
               "fact(N, F) :- N > 0, N1 is N - 1, fact(N1, F1), F is N * F1.");

  kernel_printf("fact(5, X)  -> ");
  prolog_exec_query(prolog_ctx, "fact(5, X)");

  kernel_printf("fact(7, X)  -> ");
  prolog_exec_query(prolog_ctx, "fact(7, X)");

  kfree(prolog_ctx);
}
