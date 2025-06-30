#include "forth.h"

#define stack vm->stack
#define sp vm->sp
#define dictionary vm->dictionary
#define memory vm->memory
#define dict_size vm->dict_size

#define vm_error(...)                                                          \
  {                                                                            \
    vm->io.stderr(__VA_ARGS__);                                                \
    vm->err++;                                                                 \
  }

#define vm_out(...) vm->io.stdout(__VA_ARGS__)

#define vm_in() vm->io.stdin()

#define vm_info(...)                                                           \
  {                                                                            \
    vm->io.stderr(__VA_ARGS__);                                                \
  }

void push(forth_vm_t *vm, int value) {
  if (sp < FVM_STACK_SIZE - 1)
    stack[++sp] = value;
  else
    vm_error("stack overflow!");
}

int pop(forth_vm_t *vm) {
  if (sp >= 0) {
    return stack[sp--];
  } else {
    vm_error("stack underflow!");
    return 0;
  }
}

#define FVM_OP(name, op)                                                       \
  void name(forth_vm_t *vm) {                                                  \
    if (sp < 1) {                                                              \
      vm_error("not enough values on the stack!");                             \
    } else {                                                                   \
      int r = pop(vm);                                                         \
      int l = pop(vm);                                                         \
      push(vm, l op r);                                                        \
    }                                                                          \
  }

FVM_OP(fvm_add, +)
FVM_OP(fvm_sub, -)
FVM_OP(fvm_mul, *)

#define FVM_CMP(name, op)                                                      \
  void name(forth_vm_t *vm) {                                                  \
    if (sp < 1) {                                                              \
      vm_error("not enough values for comparison!");                           \
    } else {                                                                   \
      int r = pop(vm);                                                         \
      int l = pop(vm);                                                         \
      push(vm, l op r ? 1 : 0);                                                \
    }                                                                          \
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
    vm_error("stack underflow! cannot negate.");
}

void fvm_dup(forth_vm_t *vm) {
  if (sp >= 0)
    push(vm, stack[sp]);
  else
    vm_error("stack underflow! cannot duplicate.");
}

void fvm_swap(forth_vm_t *vm) {
  if (sp < 1) {
    vm_error("not enough values to swap!");
  } else {
    int a = pop(vm), b = pop(vm);
    push(vm, a);
    push(vm, b);
  }
}

void fvm_drop(forth_vm_t *vm) {
  if (sp >= 0)
    pop(vm);
  else {
    vm_error("stack underflow! cannot drop.");
  }
}

void fvm_over(forth_vm_t *vm) {
  if (sp < 1) {
    vm_error("not enough values for over!");
  } else {
    push(vm, stack[sp - 1]);
  }
}

void fvm_div(forth_vm_t *vm) {
  if (sp < 1) {
    vm_error("not enough values on the stack for division!");
  } else {
    int b = pop(vm), a = pop(vm);
    if (b)
      push(vm, a / b);
    else
      vm_error("division by zero!");
  }
}

void fvm_print_top(forth_vm_t *vm) {
  if (sp >= 0) {
    vm_out("%d", pop(vm));
  } else
    vm_error("stack underflow! nothing to print.");
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
  if (sp < 2) {
    vm_error("not enough values to rotate!");
  } else {
    int c = pop(vm), b = pop(vm), a = pop(vm);
    push(vm, b);
    push(vm, c);
    push(vm, a);
  }
}

void fvm_nip(forth_vm_t *vm) {
  if (sp < 1) {
    vm_error("not enough values to nip!");
  } else {
    int b = pop(vm);
    pop(vm);
    push(vm, b);
  }
}

void fvm_tuck(forth_vm_t *vm) {
  if (sp < 1) {
    vm_error("not enough values to tuck!");
  } else {
    int b = pop(vm), a = pop(vm);
    push(vm, b);
    push(vm, a);
    push(vm, b);
  }
}

void fvm_store(forth_vm_t *vm) {
  if (sp < 1) {
    vm_error("not enough values for store!");
  } else {
    int addr = pop(vm), value = pop(vm);
    if (addr >= 0 && addr < 256)
      memory[addr] = value;
    else
      vm_error("invalid memory address!");
  }
}

void fvm_fetch(forth_vm_t *vm) {
  if (sp < 0) {
    vm_error("not enough values for fetch!");
  } else {
    int addr = pop(vm);
    if (addr >= 0 && addr < 256)
      push(vm, memory[addr]);
    else
      vm_error("invalid memory address!");
  }
}

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
      if (isdigit(loop_body[k][0]) ||
          (loop_body[k][0] == '-' && isdigit(loop_body[k][1]))) {
        push(vm, atoi(loop_body[k]));
      } else {
        fvm_execute(vm, loop_body[k]); // refactor these lateer
      }
    }
  } while (!pop(vm));
}

int loop_counter = 0;

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

  for (loop_counter = counter; loop_counter < limit; loop_counter++) {
    for (int k = 0; loop_body[k]; k++) {
      if (isdigit(loop_body[k][0]) ||
          (loop_body[k][0] == '-' && isdigit(loop_body[k][1]))) {
        push(vm, atoi(loop_body[k]));
      } else {
        fvm_execute(vm, loop_body[k]);
      }
    }
  }
}

void fvm_i(forth_vm_t *vm) { push(vm, loop_counter); }

void fvm_key(forth_vm_t *vm) {
  int ch = vm_in();
  push(vm, ch);
}

void fvm_emit(forth_vm_t *vm) {
  if (sp < 0) {
    vm_error("stack underflow in emit!");
  } else
    vm_out("%c", pop(vm));
}

void fvm_print_stack(forth_vm_t *vm) {
  if (sp < 0) {
    vm_error("stack is empty!");
    return;
  }

  for (int i = 0; i <= sp; i++) {
    vm_error("%d ", stack[i]);
  }
  vm_error("");
}

void fvm_init(forth_vm_t *vm) {
  vm->err = 0;
  sp = -1;
  dict_size = 1;
  vm->io.stdin = &fvm_default_stdin;
  vm->io.stdout = &fvm_default_stdout;
  vm->io.stderr = &fvm_default_stderr;

  fvm_register_word(vm, "+", fvm_add);
  fvm_register_word(vm, "-", fvm_sub);
  fvm_register_word(vm, "*", fvm_mul);
  fvm_register_word(vm, "/", fvm_div);
  fvm_register_word(vm, ".", fvm_print_top);
  fvm_register_word(vm, "dup", fvm_dup);
  fvm_register_word(vm, "swap", fvm_swap);
  fvm_register_word(vm, "drop", fvm_drop);
  fvm_register_word(vm, "over", fvm_over);
  fvm_register_word(vm, "=", fvm_eq);
  fvm_register_word(vm, "<>", fvm_neq);
  fvm_register_word(vm, "<", fvm_lt);
  fvm_register_word(vm, ">", fvm_gt);
  fvm_register_word(vm, "<=", fvm_leq);
  fvm_register_word(vm, ">=", fvm_geq);
  fvm_register_word(vm, "not", fvm_not);
  fvm_register_word(vm, "if", fvm_if);
  fvm_register_word(vm, "else", fvm_else);
  fvm_register_word(vm, "then", fvm_then);
  fvm_register_word(vm, "rot", fvm_rot);
  fvm_register_word(vm, "nip", fvm_nip);
  fvm_register_word(vm, "tuck", fvm_tuck);
  fvm_register_word(vm, "!", fvm_store);
  fvm_register_word(vm, "@", fvm_fetch);
  fvm_register_word(vm, "begin", fvm_begin);
  fvm_register_word(vm, "until", fvm_then);
  fvm_register_word(vm, "do", fvm_do);
  fvm_register_word(vm, "loop", fvm_then);
  fvm_register_word(vm, "key", fvm_key);
  fvm_register_word(vm, "emit", fvm_emit);
  fvm_register_word(vm, ".s", fvm_print_stack);
  fvm_register_word(vm, "i", fvm_i);
}

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
  vm_error("unknown word! '%s'", word);
}

void fvm_define_word(forth_vm_t *vm, char *input) {
  (void)vm;
  char *name = strtok(input, " ");
  if (!name) {
    vm_error("invalid word definition!");
    return;
  }

  if (dict_size >= FVM_DICTIONARY_SIZE) {
    vm_error("dictionary full!");
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

  vm_error("word definition missing ';'");
}

void fvm_register_word(forth_vm_t *vm, const char *name,
                       void (*function)(forth_vm_t *)) {
  if (dict_size < FVM_DICTIONARY_SIZE) {
    memcpy(dictionary[dict_size].name, name, FVM_WORD_SIZE);
    dictionary[dict_size].function = function;
    dict_size++;
  } else
    vm_error("dictionary is full!");
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
    if (strcmp(token, "\\") == 0)
      break;
    if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) {
      push(vm, atoi(token));
    } else {
      fvm_execute(vm, token);
    }
    token = strtok(NULL, " ");
  }
}

char fvm_default_stdin(void) { return 0; }

void fvm_default_stdout(const char *format, ...) {
  va_list args;
  va_start(args, format);
  _vprintf(kernel_putc, format, args);
  va_end(args);
}

void fvm_default_stderr(const char *format, ...) {
  va_list args; // make sure that the function really takes va_list and not just
                // ... bc its UB
  va_start(args, format);
  kernel_print_set_attribute(LIGHT_RED_ON_BLACK);
  _vprintf(kernel_putc, format, args);
  kernel_print_set_attribute(WHITE_ON_BLACK);
  kernel_printf("\n");
  va_end(args);
}
