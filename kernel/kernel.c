#include "boot/multiboot.h"
#include "drivers/mouse.h"
#include "drivers/screen.h"
#include "drivers/serial.h"

static uint32_t total_memory = 0;
static uint32_t usable_memory_start = 0;
static uint32_t usable_memory_end = 0;

typedef struct {
  uint32_t size;
  uint64_t addr;
  uint64_t len;
  uint32_t type;
} __attribute__((packed)) mmap_entry_t;

#define MMAP_TYPE_USABLE 1
#define MMAP_TYPE_RESERVED 2
#define MMAP_TYPE_ACPI 3
#define MMAP_TYPE_ACPI_NVS 4
#define MMAP_TYPE_BAD 5

uint32_t get_memory_size(void) { return total_memory; }

uint32_t get_usable_memory_start(void) { return usable_memory_start; }

uint32_t get_usable_memory_end(void) { return usable_memory_end; }

void init_memory_from_multiboot(multiboot_info_t *mbi) {
  if (mbi->flags & MULTIBOOT_FLAG_MEM) {
    // mem_lower: KB of memory below 1MB (typically 640KB)
    // mem_upper: KB of memory above 1MB
    total_memory = (mbi->mem_upper + 1024) * 1024;
    serial_printff("Memory: %d MB detected", total_memory / (1024 * 1024));
  }

  // better method: parse memory map
  if (mbi->flags & MULTIBOOT_FLAG_MMAP) {
    serial_printff("Memory map:");

    uint32_t highest_usable = 0;
    uint32_t lowest_usable = 0xFFFFFFFF;

    mmap_entry_t *entry = (mmap_entry_t *)mbi->mmap_addr;
    uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;

    while ((uint32_t)entry < mmap_end) {
      const char *type_str;
      switch (entry->type) {
      case MMAP_TYPE_USABLE:
        type_str = "usable";
        break;
      case MMAP_TYPE_RESERVED:
        type_str = "reserved";
        break;
      case MMAP_TYPE_ACPI:
        type_str = "ACPI";
        break;
      case MMAP_TYPE_ACPI_NVS:
        type_str = "ACPI NVS";
        break;
      case MMAP_TYPE_BAD:
        type_str = "bad";
        break;
      default:
        type_str = "unknown";
        break;
      }

      // only print low 32 bits (we're 32-bit anyway)
      uint32_t addr_lo = (uint32_t)entry->addr;
      uint32_t len_lo = (uint32_t)entry->len;

      serial_printff("  0x%x - 0x%x (%d KB) %s", addr_lo, addr_lo + len_lo,
                     len_lo / 1024, type_str);

      // track usable memory regions
      if (entry->type == MMAP_TYPE_USABLE) {
        uint32_t region_end = addr_lo + len_lo;

        if (addr_lo < lowest_usable && addr_lo >= 0x100000) {
          // ignore below 1MB, that's where kernel is
          lowest_usable = addr_lo;
        }

        if (region_end > highest_usable) {
          highest_usable = region_end;
        }
      }

      // move to next entry (size field doesn't include itself)
      entry =
          (mmap_entry_t *)((uint32_t)entry + entry->size + sizeof(entry->size));
    }

    usable_memory_start = lowest_usable;
    usable_memory_end = highest_usable;
    total_memory = highest_usable;

    serial_printff("Usable: 0x%x - 0x%x (%d MB)", usable_memory_start,
                   usable_memory_end, total_memory / (1024 * 1024));
  }
}

void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
  if (magic != MULTIBOOT_MAGIC) {
    return;
  }
  serial_init();
  init_memory_from_multiboot(mbi);

  if (vesa_init(mbi) == 0) {
    vesa_clear(0x001133);
    vesa_fill_rect(100, 100, 200, 150, 0xFF0000);
    vesa_draw_rect(100, 100, 200, 150, 0xFFFFFF);
    vesa_draw_line(0, 0, vesa_get_width() - 1, vesa_get_height() - 1, 0x00FF00);

    mouse_set_bounds(0, vesa_get_width() - 1, 0, vesa_get_height() - 1);
  } else {
    serial_printff("no VESA framebuffer, using text mode");
  }
}