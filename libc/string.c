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

void hex_to_ascii_padded(int num, char str[], int width, int zero_pad,
                         int uppercase) {
  (void)zero_pad;
  char temp[16]; // max 32-bit hex is 8 chars + '\0'
  int index = 0;

  for (int i = 28; i >= 0; i -= 4) {
    int tmp = (num >> i) & 0xF;
    temp[index++] =
        (tmp < 10) ? (tmp + '0') : (tmp - 10 + (uppercase ? 'A' : 'a'));
  }
  temp[index] = '\0';

  char *hex_start = temp;
  while (*hex_start == '0' && *(hex_start + 1) != '\0') {
    hex_start++;
  }

  int len = strlen(hex_start);

  int pad_len = (width > len) ? width - len : 0;
  int out_index = 0;

  str[out_index++] = '0';
  str[out_index++] = 'x';

  for (int i = 0; i < pad_len; i++) {
    str[out_index++] = '0';
  }

  strcpy(hex_start, str + out_index);
}

void _vprintf(void (*output_func)(char), const char *fmt, va_list args) {
  char num_buffer[32] = {0};

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;

      int zero_pad = 0;
      int width = 0;

      if (*fmt == '0') {
        zero_pad = 1;
        fmt++;
      }

      while (*fmt >= '0' && *fmt <= '9') {
        width = width * 10 + (*fmt - '0');
        fmt++;
      }

      switch (*fmt) {
      case 'd': {
        int num = va_arg(args, int);
        int_to_ascii(num, num_buffer);
        char *ptr = num_buffer;
        while (*ptr)
          output_func(*ptr++);
        break;
      }
      case 'x':
      case 'X': {
        int num = va_arg(args, int);
        int uppercase = (*fmt == 'X');
        hex_to_ascii_padded(num, num_buffer, width, zero_pad, uppercase);
        char *ptr = num_buffer;
        while (*ptr)
          output_func(*ptr++);
        break;
      }
      case 's': {
        char *str = va_arg(args, char *);
        while (*str)
          output_func(*str++);
        break;
      }
      case 'c': {
        char c = (char)va_arg(args, int);
        output_func(c);
        break;
      }
      case '%': {
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

buffer_writer_t output_buffer = {0};

static void buffer_output_func(char c) {
  if (output_buffer.index < output_buffer.max_size - 1) {
    output_buffer.buffer[output_buffer.index++] = c;
    output_buffer.buffer[output_buffer.index] = '\0';
  }
}

int vsprintf(char *buffer, int size, const char *fmt, va_list args) {
  buffer_writer_t writer = {buffer, 0, size};
  memcpy((char *)&output_buffer, (char *)&writer, sizeof(buffer_writer_t));
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

int atoi(const char *str) {
  int result = 0;
  int sign = 1;

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
    result = result * 10 + (*str - '0');
    str++;
  }

  return sign * result;
}

int isdigit(int c) { return (c >= '0' && c <= '9'); }

char *strcat(char *dest, const char *src) {
  char *ptr = dest + strlen(dest);

  while (*src) {
    *ptr++ = *src++;
  }

  *ptr = '\0';
  return dest;
}

char *strchr(const char *s, int c) {
  while (*s) {
    if (*s == (char)c) {
      return (char *)s;
    }
    s++;
  }
  return NULL;
}

static char *last_str = NULL;

char *strtok(char *str, const char *delim) {
  if (str) {
    last_str = str;
  } else if (!last_str) {
    return NULL;
  }

  while (*last_str && strchr(delim, *last_str)) {
    last_str++;
  }

  if (*last_str == '\0') {
    return NULL;
  }

  char *token_start = last_str;

  while (*last_str && !strchr(delim, *last_str)) {
    last_str++;
  }

  if (*last_str) {
    *last_str = '\0';
    last_str++;
  } else {
    last_str = NULL;
  }

  return token_start;
}
