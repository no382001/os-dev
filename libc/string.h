#pragma once
#include "libc/types.h"
#include <stdarg.h>

void int_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(const char s[]);
void backspace(char s[]);
void append(char s[], char n);
int strcmp(const char s1[], const char s2[]);
int strncmp(const char s1[], const char s2[], size_t n);
void hex_to_ascii(int n, char str[]);
char *strcpy(char *dest, const char *src);

void _vprintf(void (*output_func)(char), const char *fmt, va_list args);

typedef struct {
  char *buffer;
  int index;
  int max_size;
} buffer_writer_t;

int vsprintf(char *buffer, int size, const char *fmt, va_list args);
int sprintf(char *buffer, int size, const char *fmt, ...);

char *strchr(const char *s, int c);
char *strtok(const char *str, const char *delim);
char *strcat(char *dest, const char *src);
int isdigit(int c);
int atoi(const char *str);
void itoa(int n, char str[]);
char *strrchr(const char *s, int c);
char *strncpy(char *dest, const char *src, size_t n);
#include "libc/types.h"
#include "mem.h"
#include "string.h"
#include <stdarg.h>

int sscanf(const char *buffer, const char *fmt, ...);
int snprintf(char *buffer, size_t size, const char *fmt, ...);