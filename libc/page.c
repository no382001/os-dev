#include "libc/page.h"
#include "drivers/serial.h"
// https://web.archive.org/web/20160326061042/http://jamesmolloy.co.uk/tutorial_html/6.-Paging.html
#undef serial_debug
#define serial_debug(...)

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

// A bitset of frames - used or free.
uint32_t *frames = 0;
uint32_t nframes = 0;

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a / (8 * 4))
#define OFFSET_FROM_BIT(a) (a % (8 * 4))

// Static function to set a bit in the frames bitset
static void set_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr / 0x1000;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr / 0x1000;
  uint32_t idx = INDEX_FROM_BIT(frame);
  uint32_t off = OFFSET_FROM_BIT(frame);
  frames[idx] &= ~(0x1 << off);
}

// Static function to find the first free frame.
static uint32_t first_frame() {
  uint32_t i, j;
  for (i = 0; i < INDEX_FROM_BIT(nframes); i++) {
    if (frames[i] != 0xFFFFFFFF) // nothing free, exit early.
    {
      // at least one bit is free here.
      for (j = 0; j < 32; j++) {
        uint32_t toTest = 0x1 << j;
        if (!(frames[i] & toTest)) {
          return (i * 4 * 8 + j);
        }
      }
    }
  }
  return -1;
}

// Function to allocate a frame.
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

    set_frame(idx * 0x1000);
    page->present = 1;
    page->rw = 1;
    page->user = 0;
    page->frame = idx;
  }
}

// Function to deallocate a frame.
void free_frame(page_t *page) {
  uint32_t frame;
  if (!(frame = page->frame)) {
    return; // The given page didn't actually have an allocated frame!
  } else {
    clear_frame(frame); // Frame is now free again.
    page->frame = 0x0;  // Page now doesn't have a frame.
  }
}

page_directory_t *kernel_directory = 0;
page_directory_t *current_directory = 0;
extern uint32_t placement_address;
extern heap_t *kheap;

void initialise_paging() {
  uint32_t mem_end_page = 0x1000000;

  nframes = mem_end_page / 0x1000;
  frames = (uint32_t *)kmalloc(INDEX_FROM_BIT(nframes));
  if (!frames) {
    kernel_panic("kmalloc failed!");
  }
  memset(frames, 0, INDEX_FROM_BIT(nframes));

  // Let's make a page directory.
  kernel_directory = (page_directory_t *)kmalloc_a(sizeof(page_directory_t));
  if (!kernel_directory) {
    kernel_panic("kmalloc failed!");
  }
  memset(kernel_directory, 0, sizeof(page_directory_t));
  current_directory = kernel_directory;

  // We need to identity map (phys addr = virt addr) from
  // 0x0 to the end of used memory, so we can access this
  // transparently, as if paging wasn't enabled.
  // NOTE that we use a while loop here deliberately.
  // inside the loop body we actually change placement_address
  // by calling kmalloc(). A while loop causes this to be
  // computed on-the-fly rather than once at the start.
  uint32_t i = 0;
  for (i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000)
    get_page(i, 1, kernel_directory);

  i = 0;
  // this is kinda weird. the first address it wants to give is 0x157000 but
  // since the prev 0x156000 + 0x1000 only reaches to 0x156fff, 0x157000 will be
  // unmapped and we page fault on heap creation so just create an additional
  // page as a workaround... i guess
  while (i < placement_address + 0x1000) {
    page_t *p = get_page(i, 1, kernel_directory);
    alloc_frame(p, 0, 0);
    i += 0x1000;
  }

  for (i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += 0x1000)
    alloc_frame(get_page(i, 1, kernel_directory), 0, 0);

  // https://forum.osdev.org/viewtopic.php?t=57400
  i = 0xC0040000; // bios uses this? otherwise i crash
  while (i < 0xC0050000) {
    page_t *p = get_page(i, 1, kernel_directory);
    alloc_frame(p, 0, 0);
    i += 0x1000;
  }

  register_interrupt_handler(14, page_fault);
  switch_page_directory(kernel_directory);

  kheap = create_heap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE,
                      KHEAP_INITIAL_SIZE, 0, 0);
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
  // serial_debug("getting page at %x", address);
  //  Turn the address into an index.
  address /= 0x1000;
  // Find the page table containing this address.
  uint32_t table_idx = address / 1024;
  if (dir->tables[table_idx]) // If this table is already assigned
  {
    // serial_debug("table found! %d", table_idx);
    return &dir->tables[table_idx]->pages[address % 1024];
  } else if (make) {
    uint32_t tmp;
    serial_debug("making a table...");
    dir->tables[table_idx] =
        (page_table_t *)kmalloc_ap(sizeof(page_table_t), &tmp);
    memset(dir->tables[table_idx], 0, sizeof(page_table_t));
    dir->tablesPhysical[table_idx] = tmp | 0x3; // PRESENT, RW
    return &dir->tables[table_idx]->pages[address % 1024];
  } else {
    // serial_debug("table not found!");
    return 0;
  }
}

void print_mapped_pages(page_directory_t *dir) {
  uint32_t start = 0;
  uint32_t end = 0;
  int in_range = 0;

  serial_printff("mapped addresses:");
  for (uint32_t i = 0; i < 1024; i++) {
    if (!dir->tables[i]) {
      continue;
    }

    uint32_t page_table_base = i * 1024 * 0x1000;

    for (uint32_t j = 0; j < 1024; j++) {
      page_t *page = &dir->tables[i]->pages[j];

      if (page->present) {
        uint32_t addr = page_table_base + (j * 0x1000);

        if (!in_range) {
          start = addr;
          in_range = 1;
        }
        end = addr;
      } else if (in_range) {
        serial_printff("   %x - %x", start, end + 0x1000 - 1);
        in_range = 0;
      }
    }
  }

  if (in_range) {
    serial_printff("   %x - %x", start, end + 0x1000 - 1);
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
  print_mapped_pages(kernel_directory);
  while (1) {
  }
}