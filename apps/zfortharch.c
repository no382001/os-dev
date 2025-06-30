#include "drivers/screen.h"
#include "libc/types.h"
#include "zforth.h"

zf_input_state zf_host_sys(zf_ctx *ctx, zf_syscall_id id, const char *input) {
  kernel_printf("zforth syscall: %d\n", id);
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

  case ZF_SYSCALL_USER: {
    break;
  }

  default:
    kernel_printf("unknown zforth syscall: %d\n", id);
    break;
  }

  return ZF_INPUT_INTERPRET;
}

void zf_host_trace(zf_ctx *ctx, const char *fmt, va_list va) {
  _vprintf(kernel_putc, fmt, va);
}
zf_cell zf_host_parse_num(zf_ctx *ctx, const char *buf) {

  if (!buf || !*buf)
    return 0;

  zf_cell result = 0;
  int negative = 0;
  const char *p = buf;

  if (*p == '-') {
    negative = 1;
    p++;
  }

  if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
    p += 2;
    while (*p) {
      if (*p >= '0' && *p <= '9') {
        result = result * 16 + (*p - '0');
      } else if (*p >= 'a' && *p <= 'f') {
        result = result * 16 + (*p - 'a' + 10);
      } else if (*p >= 'A' && *p <= 'F') {
        result = result * 16 + (*p - 'A' + 10);
      } else {
        break; // invalid hex digit
      }
      p++;
    }
  } else {
    while (*p >= '0' && *p <= '9') {
      result = result * 10 + (*p - '0');
      p++;
    }
  }

  return negative ? -result : result;
}