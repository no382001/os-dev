#pragma once
#include "bits.h"

#define FVM_STACK_SIZE 128
#define FVM_WORD_SIZE 8
#define FVM_DICTIONARY_SIZE 128

typedef struct forth_vm_t forth_vm_t;

typedef struct {
  char name[FVM_WORD_SIZE];
  void (*function)(forth_vm_t *);
  char usercode[256];
} forth_word_t;

typedef struct {
  char (*stdin)(void);
  void (*stdout)(const char *, ...);
  void (*stderr)(const char *, ...);
} forth_vm_io_t;

struct forth_vm_t {
  int stack[FVM_STACK_SIZE];
  int sp;
  forth_word_t dictionary[FVM_DICTIONARY_SIZE];
  int dict_size;
  int memory[256];
  forth_vm_io_t io;
};

void push(forth_vm_t *vm, int value);
int pop(forth_vm_t *vm);
void fvm_init(forth_vm_t *vm);
void fvm_define_word(forth_vm_t *vm, char *input);
void fvm_register_word(forth_vm_t *vm, const char *name,
                       void (*function)(forth_vm_t *));
void fvm_repl(forth_vm_t *vm, char *input);

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

char fvm_default_stdin(void);
void fvm_default_stdout(const char *, ...);
void fvm_default_stderr(const char *, ...);