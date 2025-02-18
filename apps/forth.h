#pragma once
#include "bits.h"

#define FVM_STACK_SIZE 124
#define FVM_WORD_SIZE 8
#define FVM_DICTIONARY_SIZE 124

typedef struct {
    int stack[FVM_STACK_SIZE];
    int sp;
} forth_vm_t;

typedef struct {
    char name[FVM_WORD_SIZE];
    void (*function)(forth_vm_t *);
    char usercode[256];
} forth_word_t;

void push(forth_vm_t *vm, int value);
int pop(forth_vm_t *vm);
void fvm_init(forth_vm_t *vm);
void fvm_define_word(forth_vm_t *vm, char *input);
void fvm_register_word(const char *name, void (*function)(forth_vm_t *));
void fvm_repl(forth_vm_t *vm, char *input);