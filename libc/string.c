#include "string.h"
#include "mem.h"

void itoa(int n, char str[]) { int_to_ascii(n, str); }

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

int strncmp(const char s1[], const char s2[], size_t n) {
  char temp1[n + 1];
  char temp2[n + 1];

  memcpy(temp1, s1, n);
  memcpy(temp2, s2, n);

  temp1[n] = '\0';
  temp2[n] = '\0';

  return strcmp(temp1, temp2);
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

char *strcpy(char *dest, const char *str) {
  int len = strlen(str);
  memcpy(dest, str, len + 1);
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

  strcpy(str + out_index, hex_start);
}

// unsafe, primal
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

static int isspace(char c) { return c == ' '; }

static int parse_int(const char **buffer, int base) {
  int result = 0, sign = 1;

  while (isspace(**buffer))
    (*buffer)++;

  if (**buffer == '-') {
    sign = -1;
    (*buffer)++;
  } else if (**buffer == '+') {
    (*buffer)++;
  }

  while (isdigit(**buffer) ||
         (base == 16 && (**buffer >= 'a' && **buffer <= 'f')) ||
         (base == 16 && (**buffer >= 'A' && **buffer <= 'F'))) {
    if (**buffer >= '0' && **buffer <= '9') {
      result = result * base + (**buffer - '0');
    } else if (**buffer >= 'a' && **buffer <= 'f') {
      result = result * base + (**buffer - 'a' + 10);
    } else if (**buffer >= 'A' && **buffer <= 'F') {
      result = result * base + (**buffer - 'A' + 10);
    }
    (*buffer)++;
  }

  return sign * result;
}

static float parse_float(const char **buffer) {
  float result = 0.0f;
  float divisor = 1.0f;
  int sign = 1;

  while (isspace(**buffer))
    (*buffer)++;

  if (**buffer == '-') {
    sign = -1;
    (*buffer)++;
  } else if (**buffer == '+') {
    (*buffer)++;
  }

  while (isdigit(**buffer)) {
    result = result * 10 + (**buffer - '0');
    (*buffer)++;
  }

  if (**buffer == '.') {
    (*buffer)++;
    while (isdigit(**buffer)) {
      result = result * 10 + (**buffer - '0');
      divisor *= 10.0f;
      (*buffer)++;
    }
  }

  return sign * (result / divisor);
}

int sscanf(const char *buffer, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int matched = 0;

  while (*fmt) {
    if (*fmt == '%') {
      fmt++;

      while (isspace(*buffer))
        buffer++;

      if (*fmt == 'd') {
        int *num_ptr = va_arg(args, int *);
        *num_ptr = parse_int(&buffer, 10);
        matched++;
      } else if (*fmt == 'x') {
        int *num_ptr = va_arg(args, int *);
        *num_ptr = parse_int(&buffer, 16);
        matched++;
      } else if (*fmt == 'f') {
        float *num_ptr = va_arg(args, float *);
        *num_ptr = parse_float(&buffer);
        matched++;
      } else if (*fmt == 's') {
        char *str_ptr = va_arg(args, char *);
        while (*buffer && !isspace(*buffer)) {
          *str_ptr++ = *buffer++;
        }
        *str_ptr = '\0';
        matched++;
      } else if (*fmt == 'c') {
        char *char_ptr = va_arg(args, char *);
        *char_ptr = *buffer++;
        matched++;
      }
    } else {
      if (*buffer != *fmt) {
        break;
      }
      buffer++;
    }
    fmt++;
  }

  va_end(args);
  return matched;
}

/**
 * find the last occurrence of character c in string s
 * returns pointer to the last occurrence of c in s, or NULL if not found
 */
char *strrchr(const char *s, int c) {
  const char *last = NULL;

  char ch = (char)c;

  while (*s) {
    if (*s == ch) {
      last = s;
    }
    s++;
  }

  if (ch == '\0') {
    return (char *)s; // return pointer to null terminator
  }

  return (char *)last; // return last found position
}

char *strncpy(char *dest, const char *src, size_t n) {
  size_t i;

  for (i = 0; i < n && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }

  for (; i < n; i++) {
    dest[i] = '\0';
  }

  return dest;
}