#pragma once
#include "bits.h"

void int_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(const char s[]);
void backspace(char s[]);
void append(char s[], char n);
int strcmp(const char s1[], const char s2[]);
void hex_to_ascii(int n, char str[]);
char *strcpy(const char *src, char *dest);

void _vprintf(void (*output_func)(char), const char *fmt, va_list args);

typedef struct {
  char *buffer;
  int index;
  int max_size;
} buffer_writer_t;

int vsprintf(char *buffer, int size, const char *fmt, va_list args);
int sprintf(char *buffer, int size, const char *fmt, ...);