#include "bits.h"

void isr_install() {
  kernel_printf("- installing isr\n");

  set_idt_gate(0, (uint32_t)isr0);
  set_idt_gate(1, (uint32_t)isr1);
  set_idt_gate(2, (uint32_t)isr2);
  set_idt_gate(3, (uint32_t)isr3);
  set_idt_gate(4, (uint32_t)isr4);
  set_idt_gate(5, (uint32_t)isr5);
  set_idt_gate(6, (uint32_t)isr6);
  set_idt_gate(7, (uint32_t)isr7);
  set_idt_gate(8, (uint32_t)isr8);
  set_idt_gate(9, (uint32_t)isr9);
  set_idt_gate(10, (uint32_t)isr10);
  set_idt_gate(11, (uint32_t)isr11);
  set_idt_gate(12, (uint32_t)isr12);
  set_idt_gate(13, (uint32_t)isr13);
  set_idt_gate(14, (uint32_t)isr14);
  set_idt_gate(15, (uint32_t)isr15);
  set_idt_gate(16, (uint32_t)isr16);
  set_idt_gate(17, (uint32_t)isr17);
  set_idt_gate(18, (uint32_t)isr18);
  set_idt_gate(19, (uint32_t)isr19);
  set_idt_gate(20, (uint32_t)isr20);
  set_idt_gate(21, (uint32_t)isr21);
  set_idt_gate(22, (uint32_t)isr22);
  set_idt_gate(23, (uint32_t)isr23);
  set_idt_gate(24, (uint32_t)isr24);
  set_idt_gate(25, (uint32_t)isr25);
  set_idt_gate(26, (uint32_t)isr26);
  set_idt_gate(27, (uint32_t)isr27);
  set_idt_gate(28, (uint32_t)isr28);
  set_idt_gate(29, (uint32_t)isr29);
  set_idt_gate(30, (uint32_t)isr30);
  set_idt_gate(31, (uint32_t)isr31);

  // remap pic https://wiki.osdev.org/8259_PIC
  port_byte_out(0x20, 0x11);
  port_byte_out(0xA0, 0x11);
  port_byte_out(0x21, 0x20);
  port_byte_out(0xA1, 0x28);
  port_byte_out(0x21, 0x04);
  port_byte_out(0xA1, 0x02);
  port_byte_out(0x21, 0x01);
  port_byte_out(0xA1, 0x01);
  port_byte_out(0x21, 0x0);
  port_byte_out(0xA1, 0x0);

  set_idt_gate(32, (uint32_t)irq0);
  set_idt_gate(33, (uint32_t)irq1);
  set_idt_gate(34, (uint32_t)irq2);
  set_idt_gate(35, (uint32_t)irq3);
  set_idt_gate(36, (uint32_t)irq4);
  set_idt_gate(37, (uint32_t)irq5);
  set_idt_gate(38, (uint32_t)irq6);
  set_idt_gate(39, (uint32_t)irq7);
  set_idt_gate(40, (uint32_t)irq8);
  set_idt_gate(41, (uint32_t)irq9);
  set_idt_gate(42, (uint32_t)irq10);
  set_idt_gate(43, (uint32_t)irq11);
  set_idt_gate(44, (uint32_t)irq12);
  set_idt_gate(45, (uint32_t)irq13);
  set_idt_gate(46, (uint32_t)irq14);
  set_idt_gate(47, (uint32_t)irq15);

  set_idt(); //
}

static char *exception_messages[] = {"division by zero",
                                     "debug",
                                     "non maskable interrupt",
                                     "breakpoint",
                                     "into detected overflow",
                                     "out of bounds",
                                     "invalid opcode",
                                     "no coprocessor",

                                     "double fault",
                                     "coprocessor segment overrun",
                                     "bad tss",
                                     "segment not present",
                                     "stack fault",
                                     "general protection fault",
                                     "page fault",
                                     "unknown interrupt",

                                     "coprocessor fault",
                                     "alignment check",
                                     "machine check",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",

                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved",
                                     "reserved"};

void print_registers(registers_t *r) {
  serial_puts("--- register dump ---\n");

  serial_puts("DS: ");
  serial_put_hex(r->ds);
  serial_puts("\n");

  serial_puts("EDI: ");
  serial_put_hex(r->edi);
  serial_puts(" ESI: ");
  serial_put_hex(r->esi);
  serial_puts(" EBP: ");
  serial_put_hex(r->ebp);
  serial_puts(" ESP: ");
  serial_put_hex(r->esp);
  serial_puts("\n");

  serial_puts("EBX: ");
  serial_put_hex(r->ebx);
  serial_puts(" EDX: ");
  serial_put_hex(r->edx);
  serial_puts(" ECX: ");
  serial_put_hex(r->ecx);
  serial_puts(" EAX: ");
  serial_put_hex(r->eax);
  serial_puts("\n");

  serial_puts("interrupt number: ");
  serial_put_hex(r->int_no);
  serial_puts(" error code: ");
  serial_put_hex(r->err_code);
  serial_puts("\n");

  serial_puts("EIP: ");
  serial_put_hex(r->eip);
  serial_puts(" CS: ");
  serial_put_hex(r->cs);
  serial_puts(" EFLAGS: ");
  serial_put_hex(r->eflags);
  serial_puts("\n");

  serial_puts("User ESP: ");
  serial_put_hex(r->useresp);
  serial_puts(" SS: ");
  serial_put_hex(r->ss);
  serial_puts("\n");

  serial_puts("--------------------\n");
}

void isr_handler(registers_t *r) {
  if (r->int_no > 31) {
    serial_debug(" unknown interrupt: %d", r->int_no);
    print_registers(r);
  } else {
    serial_debug(" received interrupt: %d -> %s", r->int_no,
                 exception_messages[r->int_no]);
  }

  while (1) {
    asm volatile("hlt"); // shit can go wrong here
  }
}

isr_t interrupt_handlers[256] = {0};

void register_interrupt_handler(uint8_t n, isr_t handler) {
  interrupt_handlers[n] = handler;
}

#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20 /* end-of-interrupt command code */

void irq_handler(registers_t *r) {
  if (r->int_no >= 40) // why 40?
    port_byte_out(PIC2, PIC_EOI);
  port_byte_out(PIC1, PIC_EOI);

  if (interrupt_handlers[r->int_no] != 0) {
    isr_t handler = interrupt_handlers[r->int_no];
    handler(r);
  }
}

void irq_install() {
  kernel_printf("- installing irq\n");
  asm volatile("sti");
  init_timer(50);
  init_keyboard();
}
