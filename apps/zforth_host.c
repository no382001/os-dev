#include "drivers/screen.h"
#include "drivers/serial.h"
#include "fs/vfs.h"
#include "libc/string.h"
#include "libc/types.h"
#include "zforth_core.h"

zf_input_state zf_host_sys(zf_ctx *ctx, zf_syscall_id id, const char *input) {
  switch (id) {
  case ZF_SYSCALL_EMIT: {
    zf_cell c = zf_pop(ctx);
    kernel_putc((char)c);
    break;
  }

  case ZF_SYSCALL_PRINT: {
    zf_cell n = zf_pop(ctx);
    kernel_printf("%d", (int)n);
    break;
  }

  case ZF_SYSCALL_TELL: {
    zf_cell len = zf_pop(ctx);
    zf_cell addr = zf_pop(ctx);

    for (int i = 0; i < len; i++) {
      if ((int)addr + i < ZF_DICT_SIZE) {
        kernel_putc(ctx->dict[(int)addr + i]);
      }
    }
    break;
  }

  case ZF_SYSCALL_VFS_STAT: {
    // should be 9p
    break;
  }

  default:
    kernel_printf("unknown zforth syscall: %d\n", id);
    break;
  }

  return ZF_INPUT_INTERPRET;
}

#include "drivers/serial.h"

void zf_host_trace(zf_ctx *ctx, const char *fmt, va_list va) {
  _vprintf(serial_write, fmt, va);
}

int atoi2(const char *str, int *result) {
  const char *original = str;
  int value = 0;
  int sign = 1;
  int has_digits = 0;

  while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' ||
         *str == '\f' || *str == '\v') {
    str++;
  }

  if (*str == '-') {
    sign = -1;
    str++;
  } else if (*str == '+') {
    str++;
  }

  while (*str >= '0' && *str <= '9') {
    has_digits = 1;
    value = value * 10 + (*str - '0');
    str++;
  }

  while (*str) {
    if (*str != ' ' && *str != '\t' && *str != '\n' && *str != '\r' &&
        *str != '\f' && *str != '\v') {
      return 0;
    }
    str++;
  }

  if (!has_digits) {
    return 0;
  }

  if (result) {
    *result = sign * value;
  }
  return 1;
}

// w/ some assumptions. no real float parsed
zf_cell zf_host_parse_num(zf_ctx *ctx, const char *buf) {
  int value = 0;

  if (atoi2(buf, &value)) {
    return (zf_cell)value;
  }

  zf_abort(ctx, ZF_ABORT_NOT_A_WORD);
  return 0;
}