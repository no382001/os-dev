#pragma once
#include "kernel/types.h"

/* ISRs reserved for CPU exceptions */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

/* struct which aggregates many registers */
typedef struct {
  u32 ds;                                     /* fata segment selector */
  u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pushed by pusha. */
  u32 int_no, err_code; /* interrupt number and error code (if applicable) */
  u32 eip, cs, eflags, useresp, ss; /* pushed by the processor automatically */
} registers_t;

void isr_install();
void isr_handler(registers_t r);

typedef void (*isr_t)(registers_t);
void register_interrupt_handler(u8 n, isr_t handler);