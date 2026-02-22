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
  kernel_puts((char *)str);
}

static void pl_writef(prolog_ctx_t *ctx, const char *fmt, va_list args,
                      void *ud) {
  (void)ctx;
  (void)ud;
  if (is_mode_03h())
    _vprintf(kernel_putc, fmt, args);
}

static void pl_write_term_hook(prolog_ctx_t *ctx, term_t *t, env_t *env,
                               void *ud) {
  (void)ud;
  print_term(ctx, t, env);
}

static void setup_io_hooks(prolog_ctx_t *ctx) {
  io_hooks_t hooks = {0};
  hooks.write_str = pl_write_str;
  hooks.writef = pl_writef;
  hooks.write_term = pl_write_term_hook;
  io_hooks_set(ctx, &hooks);
}

/* ── OS FFI builtins ────────────────────────────────────────────────────── */

/* ls(Path, Files) — unify Files with list of f(Name, dir|file) terms */
static int ffi_ls(prolog_ctx_t *ctx, term_t *goal, env_t *env) {
  custom_builtin_t *cb = ffi_get_builtin_userdata(ctx, goal);
  vfs *fs = (vfs *)cb->userdata;

  term_t *path_arg = deref(env, goal->args[0]);
  const char *path =
      (path_arg->type == STRING) ? path_arg->string_data : path_arg->name;

  int fd;
  if (fs->open(fs, path, VFS_READ, &fd) != VFS_SUCCESS)
    return -1;

  vfs_stat entries[64];
  int64_t count = fs->readdir(fs, fd, entries, 64);
  fs->close(fs, fd);

  if (count < 0)
    return -1;

  term_t *list = make_const(ctx, "[]");
  for (int i = (int)count - 1; i >= 0; i--) {
    term_t *name = make_const(ctx, entries[i].name);
    term_t *type =
        make_const(ctx, entries[i].type == FS_TYPE_DIRECTORY ? "dir" : "file");
    term_t *fargs[2] = {name, type};
    term_t *entry = make_func(ctx, "f", fargs, 2);
    term_t *largs[2] = {entry, list};
    list = make_func(ctx, ".", largs, 2);
  }

  return unify(ctx, goal->args[1], list, env) ? 1 : -1;
}

/* file_size(Path, Size) — unify Size with file size in bytes */
static int ffi_file_size(prolog_ctx_t *ctx, term_t *goal, env_t *env) {
  custom_builtin_t *cb = ffi_get_builtin_userdata(ctx, goal);
  vfs *fs = (vfs *)cb->userdata;

  term_t *path_arg = deref(env, goal->args[0]);
  const char *path =
      (path_arg->type == STRING) ? path_arg->string_data : path_arg->name;

  vfs_stat st;
  if (fs->stat(fs, path, &st) != VFS_SUCCESS)
    return -1;

  char buf[32];
  snprintf(buf, sizeof(buf), "%d", (int)st.size);
  return unify(ctx, goal->args[1], make_const(ctx, buf), env) ? 1 : -1;
}

/* tick(T) — unify T with current timer tick count */
static int ffi_tick(prolog_ctx_t *ctx, term_t *goal, env_t *env) {
  extern uint32_t tick;
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", (int)tick);
  return unify(ctx, goal->args[0], make_const(ctx, buf), env) ? 1 : -1;
}

/* ksleep(N) — sleep for N timer ticks */
static int ffi_ksleep(prolog_ctx_t *ctx, term_t *goal, env_t *env) {
  (void)ctx;
  term_t *arg = deref(env, goal->args[0]);
  int n = (int)pl_strtol(arg->name, 0, 10);
  sleep(n);
  return 1;
}

/* assertz(Clause) — add clause to database at runtime */
static int ffi_assertz(prolog_ctx_t *ctx, term_t *goal, env_t *env) {
  if (ctx->db_count >= MAX_CLAUSES)
    return -1;

  term_t *clause = deref(env, goal->args[0]);
  clause_t *c = &ctx->database[ctx->db_count];

  if (clause->type == FUNC && strcmp(clause->name, ":-") == 0 &&
      clause->arity == 2) {
    c->head = substitute(ctx, env, clause->args[0]);
    c->body_count = 0;
    term_t *body = deref(env, clause->args[1]);
    while (body->type == FUNC && strcmp(body->name, ",") == 0 &&
           body->arity == 2) {
      if (c->body_count < MAX_GOALS)
        c->body[c->body_count++] = substitute(ctx, env, body->args[0]);
      body = deref(env, body->args[1]);
    }
    if (c->body_count < MAX_GOALS)
      c->body[c->body_count++] = substitute(ctx, env, body);
  } else {
    c->head = substitute(ctx, env, clause);
    c->body_count = 0;
  }

  ctx->db_count++;
  return 1;
}

/* path_join(Base, Name, Result) — join path segments */
static int ffi_path_join(prolog_ctx_t *ctx, term_t *goal, env_t *env) {
  term_t *base_arg = deref(env, goal->args[0]);
  term_t *name_arg = deref(env, goal->args[1]);

  const char *base =
      (base_arg->type == STRING) ? base_arg->string_data : base_arg->name;
  const char *name =
      (name_arg->type == STRING) ? name_arg->string_data : name_arg->name;

  char buf[512];
  int base_len = strlen(base);
  if (base_len > 0 && base[base_len - 1] == '/')
    snprintf(buf, sizeof(buf), "%s%s", base, name);
  else
    snprintf(buf, sizeof(buf), "%s/%s", base, name);

  return unify(ctx, goal->args[2], make_string(ctx, buf), env) ? 1 : -1;
}

/* tree(Path) — print filesystem tree using the same renderer as boot */
static int ffi_tree(prolog_ctx_t *ctx, term_t *goal, env_t *env) {
  (void)ctx;
  custom_builtin_t *cb = ffi_get_builtin_userdata(ctx, goal);
  vfs *fs = (vfs *)cb->userdata;

  term_t *path_arg = deref(env, goal->args[0]);
  const char *path =
      (path_arg->type == STRING) ? path_arg->string_data : path_arg->name;

  vfs_print_tree(fs, path);
  return 1;
}

static int ffi_help(prolog_ctx_t *ctx, term_t *goal, env_t *env) {
  (void)goal;
  (void)env;

  kernel_printf("built-in:  ");
  for (int i = 0; builtins[i].name; i++) {
    if (builtins[i].arity < 0)
      kernel_printf("%s  ", builtins[i].name);
    else
      kernel_printf("%s/%d  ", builtins[i].name, builtins[i].arity);
  }
  kernel_printf("\n");

  if (ctx->custom_builtin_count > 0) {
    kernel_printf("ffi:       ");
    for (int i = 0; i < ctx->custom_builtin_count; i++) {
      custom_builtin_t *cb = &ctx->custom_builtins[i];
      if (cb->arity < 0)
        kernel_printf("%s  ", cb->name);
      else
        kernel_printf("%s/%d  ", cb->name, cb->arity);
    }
    kernel_printf("\n");
  }

  return 1;
}

/* listing/0 — show all clauses in the database */
static int ffi_listing(prolog_ctx_t *ctx, term_t *goal, env_t *env) {
  (void)goal;
  (void)env;

  if (ctx->db_count == 0) {
    kernel_printf("(empty)\n");
    return 1;
  }

  kernel_printf("database:  ");
  for (int i = 0; i < ctx->db_count; i++) {
    term_t *head = ctx->database[i].head;
    int arity = (head->type == CONST) ? 0 : head->arity;
    int dup = 0;
    for (int j = 0; j < i; j++) {
      term_t *prev = ctx->database[j].head;
      int prev_arity = (prev->type == CONST) ? 0 : prev->arity;
      if (arity == prev_arity && strcmp(head->name, prev->name) == 0) {
        dup = 1;
        break;
      }
    }
    if (!dup)
      kernel_printf("%s/%d  ", head->name, arity);
  }
  kernel_printf("\n");
  return 1;
}

static void register_os_builtins(prolog_ctx_t *ctx, vfs *fs) {
  ffi_register_builtin(ctx, "ls", 2, ffi_ls, fs);
  ffi_register_builtin(ctx, "file_size", 2, ffi_file_size, fs);
  ffi_register_builtin(ctx, "tick", 1, ffi_tick, NULL);
  ffi_register_builtin(ctx, "ksleep", 1, ffi_ksleep, NULL);
  ffi_register_builtin(ctx, "assertz", 1, ffi_assertz, NULL);
  ffi_register_builtin(ctx, "path_join", 3, ffi_path_join, NULL);
  ffi_register_builtin(ctx, "tree", 1, ffi_tree, fs);
  ffi_register_builtin(ctx, "help", 0, ffi_help, NULL);
  ffi_register_builtin(ctx, "listing", 0, ffi_listing, NULL);
}

/* ── Prolog REPL ────────────────────────────────────────────────────────── */

static volatile int repl_line_ready = 0;
static char repl_buf[256];
static volatile int repl_running = 0;

#define HISTORY_SIZE 16
static char repl_history[HISTORY_SIZE][256];
static int repl_history_count = 0;
static int repl_history_index = -1;

static void repl_history_add(const char *line) {
  if (!line[0])
    return;
  if (repl_history_count == HISTORY_SIZE) {
    for (int i = 0; i < HISTORY_SIZE - 1; i++) {
      int j = 0;
      while ((repl_history[i][j] = repl_history[i + 1][j]))
        j++;
    }
    repl_history_count--;
  }
  int i = 0;
  while (line[i] && i < 255) {
    repl_history[repl_history_count][i] = line[i];
    i++;
  }
  repl_history[repl_history_count][i] = '\0';
  repl_history_count++;
}

static void repl_load_history(int idx) {
  keyboard_ctx_t *kb = get_kb_ctx();
  while (kb->buffer_pos > 0) {
    kb->buffer_pos--;
    kb->key_buffer[kb->buffer_pos] = '\0';
    kernel_put_backspace();
  }
  const char *entry = repl_history[idx];
  int i = 0;
  while (entry[i] && kb->buffer_pos < 254) {
    kb->key_buffer[kb->buffer_pos++] = entry[i++];
  }
  kb->key_buffer[kb->buffer_pos] = '\0';
  kernel_puts((char *)entry);
}

static void repl_key_handler(uint8_t scancode, char ascii, int is_pressed) {
  keyboard_ctx_t *kb = get_kb_ctx();
  if (!is_pressed)
    return;

  if (scancode == KEY_UP) {
    if (repl_history_count == 0)
      return;
    if (repl_history_index == -1)
      repl_history_index = repl_history_count - 1;
    else if (repl_history_index > 0)
      repl_history_index--;
    else
      return;
    repl_load_history(repl_history_index);
    return;
  }

  if (scancode == KEY_DOWN) {
    if (repl_history_index == -1)
      return;
    if (repl_history_index < repl_history_count - 1) {
      repl_history_index++;
      repl_load_history(repl_history_index);
    } else {
      repl_history_index = -1;
      while (kb->buffer_pos > 0) {
        kb->buffer_pos--;
        kb->key_buffer[kb->buffer_pos] = '\0';
        kernel_put_backspace();
      }
    }
    return;
  }

  kb->default_handler(scancode, ascii, is_pressed);
}

static void repl_enter_handler(const char *line) {
  int i = 0;
  while (line[i] && i < (int)sizeof(repl_buf) - 1) {
    repl_buf[i] = line[i];
    i++;
  }
  repl_buf[i] = '\0';
  repl_history_add(line);
  repl_history_index = -1;
  repl_line_ready = 1;
}

static void repl_process_line(prolog_ctx_t *ctx, const char *line) {
  int len = 0;
  while (line[len])
    len++;
  if (len == 0)
    return;

  if (len >= 5 && line[0] == 'h' && line[1] == 'a' && line[2] == 'l' &&
      line[3] == 't' && line[4] == '.') {
    repl_running = 0;
    return;
  }

  parse_error_clear(ctx);

  const char *query = line;
  if (line[0] == '?' && line[1] == '-')
    query = line + 2;

  prolog_exec_query(ctx, (char *)query);
  if (parse_has_error(ctx))
    kernel_printf("error: %s\n", ctx->error.message);
}

static void load_prolog_from_vfs(prolog_ctx_t *ctx, vfs *fs, const char *path) {
  vfs_stat st;
  if (fs->stat(fs, path, &st) != VFS_SUCCESS) {
    kernel_printf("prolog: %s not found\n", path);
    return;
  }

  uint32_t size = st.size;
  if (size == 0)
    return;

  char *content = kmalloc(size + 1);
  if (!content)
    return;

  int fd;
  if (fs->open(fs, path, VFS_READ, &fd) != VFS_SUCCESS) {
    kfree(content);
    return;
  }

  int bytes_read = 0;
  fs->read(fs, fd, content, size, NULL, &bytes_read);
  fs->close(fs, fd);
  content[bytes_read] = '\0';

  /* mirror prolog_load_file: accumulate lines into clause, parse on '.' */
  char clause[16384] = {0};
  char line[1024];
  int line_len = 0;

  for (int i = 0; i <= bytes_read; i++) {
    char c = (i < bytes_read) ? content[i] : '\n';
    if (c == '\r')
      continue;
    if (c != '\n') {
      if (line_len < (int)sizeof(line) - 1)
        line[line_len++] = c;
      continue;
    }
    line[line_len] = '\0';
    line_len = 0;

    strip_line_comment(line);
    char *trimmed = line;
    while (isspace((unsigned char)*trimmed))
      trimmed++;

    if (*trimmed == '\0' && clause[0] == '\0')
      continue;
    if (clause[0] != '\0' && *trimmed != '\0')
      strncat(clause, " ", sizeof(clause) - strlen(clause) - 1);
    strncat(clause, trimmed, sizeof(clause) - strlen(clause) - 1);

    if (has_complete_clause(clause)) {
      if (strncmp(clause, "?-", 2) == 0)
        prolog_exec_query(ctx, clause + 2);
      else
        parse_clause(ctx, clause);
      clause[0] = '\0';
      if (parse_has_error(ctx)) {
        kernel_printf("prolog: error in %s: %s\n", path, ctx->error.message);
        parse_error_clear(ctx);
      }
    }
  }

  kfree(content);
}

void prolog_repl(vfs *fs) {
  prolog_ctx_t *ctx = kmalloc(sizeof(prolog_ctx_t));
  memset(ctx, 0, sizeof(prolog_ctx_t));

  setup_io_hooks(ctx);
  register_os_builtins(ctx, fs);
  load_prolog_from_vfs(ctx, fs, "/core.pl");

  keyboard_ctx_t *kb = get_kb_ctx();
  enter_handler_t old_handler = kb->enter_handler;
  kb->enter_handler = repl_enter_handler;
  key_handler_t old_key_handler = register_key_handler(repl_key_handler);

  repl_running = 1;
  kernel_printf("\nprolog repl ready - type `help.` for commands\n\n");
  kernel_printf("?- ");

  while (repl_running) {
    if (repl_line_ready) {
      repl_line_ready = 0;
      kernel_printf("\n");
      repl_process_line(ctx, repl_buf);
      if (repl_running)
        kernel_printf("?- ");
    }
  }

  kb->enter_handler = old_handler;
  register_key_handler(old_key_handler);
  kfree(ctx);
}
