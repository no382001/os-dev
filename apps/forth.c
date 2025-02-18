#include "forth.h"

#define stack vm->stack
#define sp vm->sp
#define vm_error(...) kernel_printf(__VA_ARGS__)

forth_word_t dictionary[FVM_DICTIONARY_SIZE];
int dict_size = 1;

void push(forth_vm_t *vm, int value) {
  if (sp < FVM_STACK_SIZE - 1)
    stack[++sp] = value;
  else
    vm_error("stack overflow!\n");
}

int pop(forth_vm_t *vm) {
  return (sp >= 0) ? stack[sp--] : (vm_error("stack underflow!\n"), 0);
}

#define FVM_OP(name, op)                                                       \
  void name(forth_vm_t *vm) {                                                  \
    if (sp < 1)                                                                \
      vm_error("not enough values on the stack!\n");                           \
    else                                                                       \
      push(vm, pop(vm) op pop(vm));                                            \
  }

FVM_OP(fvm_add, +)
FVM_OP(fvm_sub, -)
FVM_OP(fvm_mul, *)

#define FVM_CMP(name, op)                                                      \
  void name(forth_vm_t *vm) {                                                  \
    if (sp < 1)                                                                \
      vm_error("not enough values for comparison!\n");                         \
    else                                                                       \
      push(vm, pop(vm) op pop(vm) ? 0 : 1);                                    \
  }

FVM_CMP(fvm_eq, ==)
FVM_CMP(fvm_neq, !=)
FVM_CMP(fvm_lt, <)
FVM_CMP(fvm_gt, >)
FVM_CMP(fvm_leq, <=)
FVM_CMP(fvm_geq, >=)

void fvm_not(forth_vm_t *vm) {
  if (sp >= 0)
    stack[sp] = stack[sp] == 0 ? 1 : 0;
  else
    vm_error("stack underflow! cannot negate.\n");
}

void fvm_dup(forth_vm_t *vm) {
  if (sp >= 0)
    push(vm, stack[sp]);
  else
    vm_error("stack underflow! cannot duplicate.\n");
}

void fvm_swap(forth_vm_t *vm) {
  if (sp < 1)
    vm_error("not enough values to swap!\n");
  else {
    int a = pop(vm), b = pop(vm);
    push(vm, a);
    push(vm, b);
  }
}

void fvm_drop(forth_vm_t *vm) {
  if (sp >= 0)
    pop(vm);
  else
    vm_error("stack underflow! cannot drop.\n");
}

void fvm_over(forth_vm_t *vm) {
  if (sp < 1)
    vm_error("not enough values for over!\n");
  else
    push(vm, stack[sp - 1]);
}

void fvm_div(forth_vm_t *vm) {
  if (sp < 1)
    vm_error("not enough values on the stack for division!\n");
  else {
    int b = pop(vm), a = pop(vm);
    if (b)
      push(vm, a / b);
    else
      vm_error("division by zero!\n");
  }
}

void fvm_print_top(forth_vm_t *vm) {
  if (sp >= 0)
    kernel_printf("%d\n", pop(vm));
  else
    vm_error("stack underflow! nothing to print.\n");
}

void fvm_if(forth_vm_t *vm) {
  int condition = pop(vm);
  if (condition == 0) {
    char *token;
    while ((token = strtok(NULL, " ")) && strcmp(token, "else") &&
           strcmp(token, "then"))
      ;
  }
}

void fvm_else(forth_vm_t *vm) {
  (void)vm;
  char *token;
  while ((token = strtok(NULL, " ")) && strcmp(token, "then"))
    ;
}

void fvm_then(forth_vm_t *vm) { (void)vm; }

void fvm_rot(forth_vm_t *vm) {
  if (sp < 2)
    vm_error("not enough values to rotate!\n");
  else {
    int c = pop(vm), b = pop(vm), a = pop(vm);
    push(vm, b);
    push(vm, c);
    push(vm, a);
  }
}

void fvm_nip(forth_vm_t *vm) {
  if (sp < 1)
    vm_error("not enough values to nip!\n");
  else {
    int b = pop(vm);
    pop(vm);
    push(vm, b);
  }
}

void fvm_tuck(forth_vm_t *vm) {
  if (sp < 1)
    vm_error("not enough values to tuck!\n");
  else {
    int b = pop(vm), a = pop(vm);
    push(vm, b);
    push(vm, a);
    push(vm, b);
  }
}

int memory[256] = {0};

void fvm_store(forth_vm_t *vm) {
  if (sp < 1)
    vm_error("not enough values for store!\n");
  else {
    int addr = pop(vm), value = pop(vm);
    if (addr >= 0 && addr < 256)
      memory[addr] = value;
    else
      vm_error("invalid memory address!\n");
  }
}

void fvm_fetch(forth_vm_t *vm) {
  if (sp < 0)
    vm_error("not enough values for fetch!\n");
  else {
    int addr = pop(vm);
    if (addr >= 0 && addr < 256)
      push(vm, memory[addr]);
    else
      vm_error("invalid memory address!\n");
  }
}
void fvm_execute(forth_vm_t *vm, const char *word);
void fvm_begin(forth_vm_t *vm) {
  char *loop_body[256];
  char *token;
  int i = 0;

  while ((token = strtok(NULL, " ")) && strcmp(token, "until")) {
    loop_body[i++] = token;
  }
  loop_body[i] = NULL;

  do {
    for (int k = 0; loop_body[k]; k++) {
      fvm_execute(vm, loop_body[k]);
    }
  } while (!pop(vm));
}

void fvm_do(forth_vm_t *vm) {
  int limit = pop(vm);
  int counter = pop(vm);

  char *loop_body[256];
  char *token;
  int i = 0;

  while ((token = strtok(NULL, " ")) && strcmp(token, "loop")) {
    loop_body[i++] = token;
  }
  loop_body[i] = NULL;

  for (int j = counter; j < limit; j++) {
    for (int k = 0; loop_body[k]; k++) {
      fvm_execute(vm, loop_body[k]);
    }
  }
}

void fvm_key(forth_vm_t *vm) {
  int ch = 0; // getchar();
  push(vm, ch);
}

void fvm_emit(forth_vm_t *vm) {
  if (sp < 0)
    vm_error("stack underflow in emit!\n");
  else
    pop(vm); // putchar();
}

void fvm_cr(forth_vm_t *vm) {
  (void)vm; // putchar('\n');
}

void fvm_register_word(const char *name, void (*function)(forth_vm_t *));
void fvm_init(forth_vm_t *vm) {
  sp = -1;
  fvm_register_word("+", fvm_add);
  fvm_register_word("-", fvm_sub);
  fvm_register_word("*", fvm_mul);
  fvm_register_word("/", fvm_div);
  fvm_register_word(".", fvm_print_top);
  fvm_register_word("dup", fvm_dup);
  fvm_register_word("swap", fvm_swap);
  fvm_register_word("drop", fvm_drop);
  fvm_register_word("over", fvm_over);
  fvm_register_word("=", fvm_eq);
  fvm_register_word("<>", fvm_neq);
  fvm_register_word("<", fvm_lt);
  fvm_register_word(">", fvm_gt);
  fvm_register_word("<=", fvm_leq);
  fvm_register_word(">=", fvm_geq);
  fvm_register_word("not", fvm_not);
  fvm_register_word("if", fvm_if);
  fvm_register_word("else", fvm_else);
  fvm_register_word("then", fvm_then);
  fvm_register_word("rot", fvm_rot);
  fvm_register_word("nip", fvm_nip);
  fvm_register_word("tuck", fvm_tuck);
  fvm_register_word("!", fvm_store);
  fvm_register_word("@", fvm_fetch);
  fvm_register_word("begin", fvm_begin);
  fvm_register_word("until", fvm_then);
  fvm_register_word("do", fvm_do);
  fvm_register_word("loop", fvm_then);
  fvm_register_word("key", fvm_key);
  fvm_register_word("emit", fvm_emit);
  fvm_register_word("cr", fvm_cr);
}

void fvm_repl(forth_vm_t *vm, char *input);
void fvm_execute(forth_vm_t *vm, const char *word) {
  for (int i = 0; i < dict_size; i++) {
    if (!strcmp(dictionary[i].name, word)) {
      if (dictionary[i].function) {
        dictionary[i].function(vm);
      } else {
        fvm_repl(vm, dictionary[i].usercode);
      }

      return;
    }
  }
  vm_error("unknown word! '%s'\n", word);
}

void fvm_define_word(forth_vm_t *vm, char *input) {
  (void)vm;
  char *name = strtok(input, " ");
  if (!name) {
    vm_error("invalid word definition!\n");
    return;
  }

  if (dict_size >= FVM_DICTIONARY_SIZE) {
    vm_error("dictionary full!\n");
    return;
  }

  memcpy(dictionary[dict_size].name, name, FVM_WORD_SIZE);

  char *def = dictionary[dict_size].usercode;
  def[0] = '\0';

  char *token = strtok(input, " ");
  while (token) {
    if (!strcmp(token, ";")) {
      dict_size++;
      return;
    }
    strcat(def, token);
    strcat(def, " ");
    token = strtok(NULL, " ");
  }

  vm_error("word definition missing ';'\n");
}

void fvm_register_word(const char *name, void (*function)(forth_vm_t *)) {
  if (dict_size < FVM_DICTIONARY_SIZE) {
    memcpy(dictionary[dict_size].name, name, FVM_WORD_SIZE);
    dictionary[dict_size].function = function;
    dict_size++;
  } else
    vm_error("dictionary is full!\n");
}

void fvm_repl(forth_vm_t *vm, char *input) {
  char input_copy[256];
  memcpy(input_copy, input, sizeof(input_copy));
  input_copy[sizeof(input_copy) - 1] = '\0';

  char *token = strtok(input_copy, " ");
  if (!token)
    return;

  if (strcmp(token, ":") == 0) {
    fvm_define_word(vm, input);
    return;
  }

  while (token) {
    if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) {
      push(vm, atoi(token));
    } else {
      fvm_execute(vm, token);
    }
    token = strtok(NULL, " ");
  }
}
