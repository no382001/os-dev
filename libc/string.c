#include "string.h"
#include "mem.h"

void int_to_ascii(int n, char str[]) {
  int i, sign;
  if ((sign = n) < 0)
    n = -n;
  i = 0;
  do {
    str[i++] = n % 10 + '0';
  } while ((n /= 10) > 0);

  if (sign < 0)
    str[i++] = '-';
  str[i] = '\0';

  reverse(str);
}

void reverse(char s[]) {
  int c, i, j;
  for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

int strlen(const char s[]) {
  int i = 0;
  while (s[i] != '\0')
    ++i;
  return i;
}

void append(char s[], char n) {
  int len = strlen(s);
  s[len] = n;
  s[len + 1] = '\0';
}

void backspace(char s[]) {
  int len = strlen(s);
  s[len - 1] = '\0';
}

/* returns <0 if s1<s2, 0 if s1==s2, >0 if s1>s2 */
int strcmp(const char s1[], const char s2[]) {
  int i;
  for (i = 0; s1[i] == s2[i]; i++) {
    if (s1[i] == '\0')
      return 0;
  }
  return s1[i] - s2[i];
}

void hex_to_ascii(int n, char str[]) {
  str[0] = '0';
  str[1] = 'x';
  str[2] = '\0';

  int started = 0;

  for (int i = 28; i >= 0; i -= 4) {
    int tmp = (n >> i) & 0xF;

    if (tmp == 0 && !started) {
      continue;
    }

    started = 1;

    char hex_char = (tmp < 10) ? (tmp + '0') : (tmp - 10 + 'a');
    append(str, hex_char);
  }

  if (!started)
    append(str, '0');
}

char *strcpy(const char *str, char *dest) {
  int len = strlen(str);
  memcpy(dest, str, len);
  return dest;
}

void _vprintf(void (*output_func)(char), const char *fmt, va_list args) {
  char num_buffer[32] = {0};

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
      case 'd': { // integer
        int num = va_arg(args, int);
        int_to_ascii(num, num_buffer);
        char *ptr = num_buffer;
        while (*ptr)
          output_func(*ptr++);
        break;
      }
      case 'x': { // hexadecimal
        int num = va_arg(args, int);
        hex_to_ascii(num, num_buffer);
        char *ptr = num_buffer;
        while (*ptr)
          output_func(*ptr++);
        break;
      }
      case 's': { // string
        char *str = va_arg(args, char *);
        while (*str)
          output_func(*str++);
        break;
      }
      case 'c': { // character
        char c = (char)va_arg(args, int);
        output_func(c);
        break;
      }
      case '%': { // literal '%'
        output_func('%');
        break;
      }
      default:
        output_func('?');
        break;
      }
    } else {
      output_func(*fmt);
    }
    fmt++;
  }
}

static buffer_writer_t *current_buffer;
static void buffer_output_func(char c) {
  if (current_buffer->index < current_buffer->max_size - 1) {
    current_buffer->buffer[current_buffer->index++] = c;
    current_buffer->buffer[current_buffer->index] = '\0';
  }
}

int vsprintf(char *buffer, int size, const char *fmt, va_list args) {
  buffer_writer_t writer = {buffer, 0, size};
  current_buffer = &writer;
  _vprintf(buffer_output_func, fmt, args);
  return writer.index;
}

int sprintf(char *buffer, int size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int written = vsprintf(buffer, size, fmt, args);
  va_end(args);
  return written;
}
