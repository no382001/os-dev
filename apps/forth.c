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
  { vm->io.stderr(__VA_ARGS__); }

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

  fvm_register_word(vm, "+", "( a b -- a+b ) Add two numbers", fvm_add);
  fvm_register_word(vm, "-", "( a b -- a-b ) Subtract b from a", fvm_sub);
  fvm_register_word(vm, "*", "( a b -- a*b ) Multiply two numbers", fvm_mul);
  fvm_register_word(vm, "/", "( a b -- a/b ) Divide a by b", fvm_div);
  fvm_register_word(vm, "dup", "( a -- a a ) Duplicate top of stack", fvm_dup);
  fvm_register_word(vm, "swap", "( a b -- b a ) Swap top two items", fvm_swap);
  fvm_register_word(vm, "drop", "( a -- ) Remove top item from stack",
                    fvm_drop);
  fvm_register_word(vm, "over", "( a b -- a b a ) Copy second item to top",
                    fvm_over);
  fvm_register_word(vm, "rot", "( a b c -- b c a ) Rotate top three items",
                    fvm_rot);
  fvm_register_word(vm, "nip", "( a b -- b ) Remove second item", fvm_nip);
  fvm_register_word(vm, "tuck", "( a b -- b a b ) Copy top under second",
                    fvm_tuck);
  fvm_register_word(vm, "=", "( a b -- flag ) test equality", fvm_eq);
  fvm_register_word(vm, "<>", "( a b -- flag ) test inequality", fvm_neq);
  fvm_register_word(vm, "<", "( a b -- flag ) test less than", fvm_lt);
  fvm_register_word(vm, ">", "( a b -- flag ) test greater than", fvm_gt);
  fvm_register_word(vm, "<=", "( a b -- flag ) test less or equal", fvm_leq);
  fvm_register_word(vm, ">=", "( a b -- flag ) test greater or equal", fvm_geq);
  fvm_register_word(vm, "not", "( flag -- !flag ) logical NOT", fvm_not);
  fvm_register_word(vm, "if", "( flag -- ) begin conditional execution",
                    fvm_if);
  fvm_register_word(vm, "else", "alternative branch for if", fvm_else);
  fvm_register_word(vm, "then", "rnd conditional execution", fvm_then);
  fvm_register_word(vm, "begin", "start unconditional loop", fvm_begin);
  fvm_register_word(vm, "until", "( flag -- ) end loop when flag is true",
                    fvm_then);
  fvm_register_word(vm, "do", "( limit start -- ) begin counted loop", fvm_do);
  fvm_register_word(vm, "loop", "end counted loop", fvm_then);
  fvm_register_word(vm, "i", "( -- n ) current loop counter", fvm_i);
  fvm_register_word(vm, "!", "( value addr -- ) store value at address",
                    fvm_store);
  fvm_register_word(vm, "@", "( addr -- value ) fetch value from address",
                    fvm_fetch);
  fvm_register_word(vm, ".", "( n -- ) print number and remove from stack",
                    fvm_print_top);
  fvm_register_word(vm, "key", "( -- char ) read character from input",
                    fvm_key);
  fvm_register_word(vm, "emit", "( char -- ) output character", fvm_emit);
  fvm_register_word(vm, "cr", "output carriage return", fvm_cr);
  fvm_register_word(vm, "space", "output single space", fvm_space);
  fvm_register_word(vm, "spaces", "( n -- ) output n spaces", fvm_spaces);
  fvm_register_word(vm, "s\"", "string literal", NULL);
  fvm_register_word(vm, ".\"", "( str_id -- ) print string", fvm_string_print);
  fvm_register_word(vm, "strcat", "( str1 str2 -- str3 ) concatenate strings",
                    fvm_string_concat);
  fvm_register_word(vm, ".s", "display entire stack contents", fvm_print_stack);
  fvm_register_word(vm, "words", "( -- ) show all words and documentation",
                    fvm_help);
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

void fvm_define_word(forth_vm_t *vm, const char *input) {
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

int fvm_alloc_string(forth_vm_t *vm, const char *str) {
  for (int i = 0; i < FVM_STRING_POOL_SIZE; i++) {
    if (!vm->string_pool[i].in_use) {
      strncpy(vm->string_pool[i].data, str, FVM_STRING_MAX - 1);
      vm->string_pool[i].data[FVM_STRING_MAX - 1] = '\0';
      vm->string_pool[i].length = strlen(str);
      vm->string_pool[i].in_use = 1;
      return i;
    }
  }
  vm_error("string pool full!");
  return -1;
}

void fvm_free_string(forth_vm_t *vm, int string_id) {
  if (string_id >= 0 && string_id < FVM_STRING_POOL_SIZE) {
    vm->string_pool[string_id].in_use = 0;
  }
}

void fvm_string_literal(forth_vm_t *vm, const char *str) {
  int string_id = fvm_alloc_string(vm, str);
  push(vm, string_id);
}

// String operations
void fvm_string_print(forth_vm_t *vm) {
  int string_id = pop(vm);
  if (string_id >= 0 && string_id < FVM_STRING_POOL_SIZE &&
      vm->string_pool[string_id].in_use) {
    vm_out("%s", vm->string_pool[string_id].data);
  } else {
    vm_error("invalid string!");
  }
}

void fvm_string_concat(forth_vm_t *vm) {
  int str2_id = pop(vm);
  int str1_id = pop(vm);

  if (str1_id >= 0 && str2_id >= 0 && str1_id < FVM_STRING_POOL_SIZE &&
      str2_id < FVM_STRING_POOL_SIZE) {

    char combined[FVM_STRING_MAX];
    snprintf(combined, FVM_STRING_MAX, "%s%s", vm->string_pool[str1_id].data,
             vm->string_pool[str2_id].data);

    int result_id = fvm_alloc_string(vm, combined);
    push(vm, result_id);
  } else {
    vm_error("invalid strings for concatenation!");
  }
}

void fvm_cr(forth_vm_t *vm) {
  (void)vm;
  vm_out("\n");
}

void fvm_space(forth_vm_t *vm) {
  (void)vm;
  vm_out(" ");
}

void fvm_spaces(forth_vm_t *vm) { // spaces ( n -- )
  int n = pop(vm);
  for (int i = 0; i < n; i++) {
    vm_out(" ");
  }
}

void fvm_help(forth_vm_t *vm) {
  vm_out("available words:\n");
  for (int i = 0; i < dict_size; i++) {
    kernel_print_set_attribute(CYAN_ON_BLACK);
    vm_out("  %s", dictionary[i].name);
    kernel_print_set_attribute(WHITE_ON_BLACK);
    vm_out(" %s\n", dictionary[i].doc);
  }
}

void fvm_register_word(forth_vm_t *vm, const char *name, const char *doc,
                       void (*function)(forth_vm_t *)) {
  if (dict_size < FVM_DICTIONARY_SIZE) {
    memcpy(dictionary[dict_size].name, name, FVM_WORD_SIZE);
    memcpy(dictionary[dict_size].doc, doc ? doc : "", FVM_DOC_SIZE);
    dictionary[dict_size].function = function;
    dict_size++;
  } else
    vm_error("dictionary is full!");
}

static int fvm_parse_string(char *input, int start_pos, char *output,
                            int max_len) {
  int i = start_pos + 1;
  int out_pos = 0;

  while (input[i] && input[i] != '"' && out_pos < max_len - 1) {
    if (input[i] == '\\' && input[i + 1]) {
      i++;
      switch (input[i]) {
      case 'n':
        output[out_pos++] = '\n';
        break;
      case 't':
        output[out_pos++] = '\t';
        break;
      case 'r':
        output[out_pos++] = '\r';
        break;
      case '\\':
        output[out_pos++] = '\\';
        break;
      case '"':
        output[out_pos++] = '"';
        break;
      default:
        output[out_pos++] = '\\';
        output[out_pos++] = input[i];
        break;
      }
    } else {
      output[out_pos++] = input[i];
    }
    i++;
  }

  output[out_pos] = '\0';

  return (input[i] == '"') ? i + 1 : -1;
}

void fvm_repl(forth_vm_t *vm, const char *input) {
  if (!input || strlen(input) == 0)
    return;

  if (input[0] == ':' && (input[1] == ' ' || input[1] == '\t')) {
    fvm_define_word(vm, input + 1);
    return;
  }

  int pos = 0;
  int len = strlen(input);

  while (pos < len) {
    while (pos < len &&
           (input[pos] == ' ' || input[pos] == '\t' || input[pos] == '\n')) {
      pos++;
    }

    if (pos >= len)
      break;

    if (input[pos] == '\\') {
      break;
    }

    if (input[pos] == '"') {
      char string_content[FVM_STRING_MAX];
      int new_pos =
          fvm_parse_string(input, pos, string_content, FVM_STRING_MAX);

      if (new_pos == -1) {
        vm_error("unterminated string literal!");
        return;
      }

      int string_id = fvm_alloc_string(vm, string_content);
      if (string_id >= 0) {
        push(vm, string_id);
        push(vm, -1);
      }

      pos = new_pos;
      continue;
    }

    // word or number
    int token_start = pos;
    while (pos < len && input[pos] != ' ' && input[pos] != '\t' &&
           input[pos] != '\n' && input[pos] != '"') {
      pos++;
    }

    if (pos > token_start) {
      // Extract token
      char token[64];
      uint32_t token_len = pos - token_start;
      if (token_len >= sizeof(token))
        token_len = sizeof(token) - 1;

      strncpy(token, input + token_start, token_len);
      token[token_len] = '\0';

      if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) {
        push(vm, atoi(token));
      } else {
        fvm_execute(vm, token);
        if (vm->err)
          return;
      }
    }
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
