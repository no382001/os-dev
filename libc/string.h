#pragma once
#include "bits.h"

void int_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(const char s[]);
void backspace(char s[]);
void append(char s[], char n);
int strcmp(char s1[], char s2[]);
void hex_to_ascii(int n, char str[]);
char *strcpy(const char *src, char *dest);

void _vprintf(void (*output_func)(char), const char *fmt, va_list args);