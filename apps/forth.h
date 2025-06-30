#pragma once
#include "bits.h"

#define FVM_STACK_SIZE 128
#define FVM_WORD_SIZE 8
#define FVM_DOC_SIZE 128
#define FVM_DICTIONARY_SIZE 128

typedef struct forth_vm_t forth_vm_t;

typedef struct {
  char name[FVM_WORD_SIZE];
  char doc[FVM_DOC_SIZE];
  void (*function)(forth_vm_t *);
  char usercode[256];
} forth_word_t;

typedef struct {
  char (*stdin)(void);
  void (*stdout)(const char *, ...);
  void (*stderr)(const char *, ...);
} forth_vm_io_t;

#define FVM_STRING_MAX 256
#define FVM_STRING_POOL_SIZE 32

typedef struct {
  char data[FVM_STRING_MAX];
  int length;
  int in_use;
} forth_string_t;

struct forth_vm_t {
  int stack[FVM_STACK_SIZE];
  int sp;
  forth_word_t dictionary[FVM_DICTIONARY_SIZE];
  forth_string_t string_pool[FVM_STRING_POOL_SIZE];
  int next_string_id;
  int dict_size;
  int memory[256];
  forth_vm_io_t io;
  int err;
};

void push(forth_vm_t *vm, int value);
int pop(forth_vm_t *vm);
void fvm_init(forth_vm_t *vm);
void fvm_define_word(forth_vm_t *vm, const char *input);
void fvm_register_word(forth_vm_t *vm, const char *name, const char *doc,
                       void (*function)(forth_vm_t *));
void fvm_repl(forth_vm_t *vm, const char *input);

void fvm_not(forth_vm_t *vm);
void fvm_dup(forth_vm_t *vm);
void fvm_swap(forth_vm_t *vm);
void fvm_drop(forth_vm_t *vm);
void fvm_over(forth_vm_t *vm);
void fvm_div(forth_vm_t *vm);
void fvm_print_top(forth_vm_t *vm);
void fvm_if(forth_vm_t *vm);
void fvm_else(forth_vm_t *vm);
void fvm_then(forth_vm_t *vm);
void fvm_rot(forth_vm_t *vm);
void fvm_nip(forth_vm_t *vm);
void fvm_tuck(forth_vm_t *vm);
void fvm_store(forth_vm_t *vm);
void fvm_fetch(forth_vm_t *vm);
void fvm_begin(forth_vm_t *vm);
void fvm_do(forth_vm_t *vm);
void fvm_key(forth_vm_t *vm);
void fvm_emit(forth_vm_t *vm);
void fvm_cr(forth_vm_t *vm);
void fvm_execute(forth_vm_t *vm, const char *word);

int fvm_alloc_string(forth_vm_t *vm, const char *str);
void fvm_free_string(forth_vm_t *vm, int string_id);
void fvm_string_literal(forth_vm_t *vm, const char *str);
void fvm_string_print(forth_vm_t *vm);
void fvm_string_concat(forth_vm_t *vm);
void fvm_cr(forth_vm_t *vm);
void fvm_space(forth_vm_t *vm);
void fvm_spaces(forth_vm_t *vm);

void fvm_help(forth_vm_t *vm);

char fvm_default_stdin(void);
void fvm_default_stdout(const char *, ...);
void fvm_default_stderr(const char *, ...);