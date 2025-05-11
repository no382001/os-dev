#include "libc/page.h"
#include "drivers/serial.h"
// https://web.archive.org/web/20160326061042/http://jamesmolloy.co.uk/tutorial_html/6.-Paging.html

/** /
#undef serial_debug
#define serial_debug(...)
/**/

#define kernel_panic(...)                                                      \
  do {                                                                         \
    serial_debug(__VA_ARGS__);                                                 \
    while (1) {                                                                \
    }                                                                          \
  } while (0)

unsigned int is_paging_enabled() {
  unsigned int cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  return (cr0 & 0x80000000);
}

uint32_t *frames = 0;
uint32_t nframes = 0;

#define INDEX_FROM_BIT(a) (a / (8 * 4))
#define OFFSET_FROM_BIT(a) (a % (8 * 4))

static void set_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr / PAGE_SIZE;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  frames[idx] |= (0x1 << off);
}

static void clear_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr / PAGE_SIZE;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  frames[idx] &= ~(0x1 << off);
}

static uint32_t first_frame() { // free frame
  uint32_t i, j;
  for (i = 0; i < INDEX_FROM_BIT(nframes); i++) {
    if (frames[i] != 0xFFFFFFFF) // nothing free, exit early.
    {
      // at least one bit is free
      for (j = 0; j < 32; j++) {
        uint32_t toTest = 0x1 << j; // TODO shift out of bounds
        if (!(frames[i] & toTest)) {
          return (i * 4 * 8 + j);
        }
      }
    }
  }
  return -1;
}

void alloc_frame(page_t *page, int is_kernel, int is_writeable) {
  (void)is_kernel;
  (void)is_writeable;

  if (!page) {
    serial_debug("nullptr page");
    return;
  }
  if (page->frame != 0) {
    serial_debug("frame already occupied! %x", page);
    return;
  } else {
    uint32_t idx = first_frame();
    if (idx == (uint32_t)-1) {
      kernel_panic("no free frames! out of %d", nframes);
      return;
    }

    set_frame(idx * PAGE_SIZE);
    page->present = 1;
    page->rw = 1;
    page->user = 0;
    page->frame = idx;
  }
}

void free_frame(page_t *page) {
  uint32_t frame;
  if (!(frame = page->frame)) {
    return;
  } else {
    clear_frame(frame);
    page->frame = 0x0;
  }
}

page_directory_t *kernel_directory = 0;
page_directory_t *current_directory = 0;
extern uint32_t placement_address;

void print_mapped_pages(page_directory_t *dir);

void initialise_paging() {
  uint32_t mem_end_page = END_OF_MEMORY;

  nframes = mem_end_page / PAGE_SIZE;
  frames = (uint32_t *)kmalloc(INDEX_FROM_BIT(nframes));
  memset(frames, 0, INDEX_FROM_BIT(nframes));

  kernel_directory = (page_directory_t *)kmalloc_a(sizeof(page_directory_t));
  memset(kernel_directory, 0, sizeof(page_directory_t));
  current_directory = kernel_directory;

  // ALL OF THESE ARE IDENTITY MAPS!

  // pre-install pages for heap
  uint32_t i = 0;
  for (i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += PAGE_SIZE)
    get_page(i, 1, kernel_directory);

  i = 0;
  // this is kinda weird. the first address it wants to give is 0x157000 but
  // since the prev 0x156000 + PAGE_SIZE only reaches to 0x156fff, 0x157000 will
  // be unmapped and we page fault on heap creation so just create an additional
  // page as a workaround... i guess
  while (i < placement_address + PAGE_SIZE) {
    page_t *p = get_page(i, 1, kernel_directory);
    alloc_frame(p, 0, 0);
    i += PAGE_SIZE;
  }

  // alloc pages for heap
  for (i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += PAGE_SIZE)
    alloc_frame(get_page(i, 1, kernel_directory), 0, 0);

  // https://forum.osdev.org/viewtopic.php?t=57400
  i = 0xC0040000; // qemu bios uses these addresses? otherwise i page fault
  while (i < 0xC0050000) {
    page_t *p = get_page(i, 1, kernel_directory);
    alloc_frame(p, 0, 0);
    i += PAGE_SIZE;
  }

  register_interrupt_handler(14, page_fault);
  switch_page_directory(kernel_directory);

  print_mapped_pages(kernel_directory);
}

void switch_page_directory(page_directory_t *dir) {
  current_directory = dir;
  asm volatile("mov %0, %%cr3" ::"r"(&dir->tablesPhysical));
  uint32_t cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  cr0 |= 0x80000000;
  asm volatile("mov %0, %%cr0" ::"r"(cr0));
}

page_t *get_page(uint32_t address, int make, page_directory_t *dir) {

  address /= PAGE_SIZE;
  uint32_t table_idx = address / 1024;
  if (dir->tables[table_idx]) {
    return &dir->tables[table_idx]->pages[address % 1024];
  } else if (make) {
    uint32_t tmp;
    dir->tables[table_idx] =
        (page_table_t *)kmalloc_ap(sizeof(page_table_t), &tmp);
    memset(dir->tables[table_idx], 0, sizeof(page_table_t));
    dir->tablesPhysical[table_idx] = tmp | 0x3; // PRESENT, RW
    return &dir->tables[table_idx]->pages[address % 1024];
  } else {
    return 0;
  }
}

void print_mapped_pages(page_directory_t *dir) {
  uint32_t start = 0;
  uint32_t end = 0;
  int in_range = 0;

  kernel_printf("vmmap:\n");
  for (uint32_t i = 0; i < 1024; i++) {
    if (!dir->tables[i]) {
      continue;
    }

    uint32_t page_table_base = i * 1024 * PAGE_SIZE;

    for (uint32_t j = 0; j < 1024; j++) {
      page_t *page = &dir->tables[i]->pages[j];

      if (page->present) {
        uint32_t addr = page_table_base + (j * PAGE_SIZE);

        if (!in_range) {
          start = addr;
          in_range = 1;
        }
        end = addr;
      } else if (in_range) {
        kernel_printf(" %x - %x\n", start, end + PAGE_SIZE - 1);
        in_range = 0;
      }
    }
  }

  if (in_range) {
    kernel_printf(" %x - %x\n", start, end + PAGE_SIZE - 1);
  }
}

void page_fault(registers_t *regs) {
  uint32_t faulting_address;
  asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

  int present = (regs->err_code & 0x1);
  int rw = regs->err_code & 0x2; // 1 write, 0 read
  int us = regs->err_code & 0x4; // user mode?
  int reserved =
      regs->err_code & 0x8; // overwritten CPU-reserved bits of page entry?
  int id = regs->err_code & 0x10; // caused by an instruction fetch?

  char state[128] = {0};

  if (present) {
    strcat(state, "present ");
  } else {
    strcat(state, "not-present ");
  }
  if (rw) {
    strcat(state, "write ");
  } else {
    strcat(state, "ro ");
  }
  if (us) {
    strcat(state, "user-mode ");
  } else {
    strcat(state, "kernel-mode ");
  }

  if (reserved) {
    strcat(state, "reserved ");
  }
  if (id) {
    strcat(state, "instruction-fetch ");
  }

  serial_printff("page fault! ( %s) at %x", state, faulting_address);
  kernel_printf(";;;;;;;;;;;;;;;;;\n");
  kernel_printf("page fault! ( %s) at %x\n", state, faulting_address);
  print_mapped_pages(kernel_directory);
  kernel_printf(";;;;;;;;;;;;;;;;;\n");
  while (1) {
  }
}

uint32_t virtual2phys(page_directory_t *dir, void *virtual_addr) {
  uint32_t address = (uint32_t)virtual_addr;
  uint32_t page_idx = address / PAGE_SIZE;
  uint32_t table_idx = page_idx / 1024;
  uint32_t page_offset = page_idx % 1024;
  uint32_t offset_in_page = address % PAGE_SIZE;

  if (!dir->tables[table_idx]) {
    serial_printff("Attempt to translate invalid virtual address %x",
                   virtual_addr);
    return 0; // Error case
  }

  page_t *page = &dir->tables[table_idx]->pages[page_offset];

  if (!page->present) {
    serial_printff("Attempt to translate non-present page at %x", virtual_addr);
    return 0; // Error case
  }

  return (page->frame * PAGE_SIZE) + offset_in_page;
}

uint32_t virt2phys(void *virtual_addr) {
  return virtual2phys(kernel_directory, virtual_addr);
}