
#include "setjmp.h"
int setjmp(jmp_buf env) {
  asm volatile("movl %%esp, %%eax\n"
               "movl %%eax, 0(%0)\n" // Save ESP
               "movl %%ebp, %%eax\n"
               "movl %%eax, 4(%0)\n" // Save EBP
               "movl (%%esp), %%eax\n"
               "movl %%eax, 8(%0)\n" // Save return address (EIP)
               "pushf\n"
               "popl %%eax\n"
               "movl %%eax, 12(%0)\n" // Save flags
               "movl %%ebx, 16(%0)\n" // Save EBX
               "movl %%esi, 20(%0)\n" // Save ESI
               "movl %%edi, 24(%0)\n" // Save EDI
               :
               : "r"(env)
               : "eax");
  return 0;
}

void longjmp(jmp_buf env, int val) {
  if (val == 0)
    val = 1;

  asm volatile("movl 16(%0), %%ebx\n" // Restore EBX
               "movl 20(%0), %%esi\n" // Restore ESI
               "movl 24(%0), %%edi\n" // Restore EDI
               "movl 4(%0), %%ebp\n"  // Restore EBP
               "movl 0(%0), %%esp\n"  // Restore ESP
               "pushl 12(%0)\n"       // Push saved flags
               "popf\n"               // Restore flags
               "movl %1, %%eax\n"     // Set return value
               "jmp *8(%0)\n"         // Jump to saved EIP
               :
               : "r"(env), "m"(val)
               : "eax");
}