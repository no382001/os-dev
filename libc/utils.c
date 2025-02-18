#include "utils.h"
#include "bits.h"

uint32_t get_stack_usage() {
  uint32_t stack_base = 0x90000; // set in boot.asm
  uint32_t esp;
  asm volatile("mov %%esp, %0" : "=r"(esp));
  return stack_base - esp;
}

void print_stack_info() {
  uint32_t esp;
  asm volatile("mov %%esp, %0" : "=r"(esp));

  serial_printff("-------- stack ----------");
  serial_printff("used: %d bytes", get_stack_usage());
  serial_printff("left: %d bytes", esp - STACK_BOTTOM);
  serial_printff("-------------------------");
}

void print_fat_bpb(fat_bpb_t *bpb) {
    serial_puts("--- FAT16 BIOS Parameter Block ---\n");
  
    serial_printff("bytes per Sector: %d", bpb->bytes_per_sector);
    serial_printff("sectors per Cluster: %d", bpb->sectors_per_cluster);
    serial_printff("reserved Sectors: %d", bpb->reserved_sectors);
    serial_printff("fAT Count: %d", bpb->fat_count);
    serial_printff("root Directory Entries: %d", bpb->root_entries);
    serial_printff("total Sectors: %d", bpb->total_sectors);
    serial_printff("media Descriptor: 0x%x", bpb->media_descriptor);
    serial_printff("sectors per FAT: %d", bpb->sectors_per_fat);
    serial_printff("sectors per Track: %d", bpb->sectors_per_track);
    serial_printff("number of Heads: %d", bpb->heads);
    serial_printff("hidden Sectors: %d", bpb->hidden_sectors);
    serial_printff("large Sector Count: %d", bpb->large_sector_count);
  
    serial_puts("---------------------------------\n");
  }