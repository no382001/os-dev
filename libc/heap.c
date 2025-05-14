#include "heap.h"
#include "page.h"

// adapted from
// https://git.9front.org/plan9front/plan9front/0861d0d0f283a9917721214fa3dc1c51a778213d/sys/src/9/port/xalloc.c/f.html
// i have no idea what license it is

/*
#undef serial_printff
#define serial_printff(...)
*/
#undef offsetof
#define offsetof(a, b) ((int)(&(((a *)(0))->b)))
#define nil 0
#define nelem(x) (sizeof(x) / sizeof((x)[0]))

uintptr_t paddr(void *va) {
  if ((uintptr_t)va >= KHEAP_START)
    return (uintptr_t)va - KHEAP_START;
  assert(0 && "kernel pannic!");
  return 0;
}

void *kaddr(uintptr_t pa) {
  if (pa < (uintptr_t)-KHEAP_START)
    return (void *)(pa + KHEAP_START);
  assert(0 && "kernel pannic!");
  return 0;
}

enum {
  nhole = 8,
  magichole = 0x484F4C45, /* HOLE */
};

typedef struct hole_t hole_t;
typedef struct heap_allocator_t heap_allocator_t;
typedef struct block_header_t block_header_t;

struct hole_t {
  uintptr_t addr;
  uintptr_t size;
  uintptr_t top;
  hole_t *link;
};

struct block_header_t {
  uint16_t size;
  uint32_t magix;
  char data[]; // needs to be [] for offsetof
};

struct heap_allocator_t {
  hole_t hole[nhole];
  hole_t *flist;
  hole_t *table;
};

static heap_allocator_t xlists;
void *kheap = NULL;

void xinit(void) {
  hole_t *h, *eh;

  eh = &xlists.hole[nhole - 1];
  for (h = xlists.hole; h < eh; h++)
    h->link = h + 1;

  h->link = NULL; // ensure the last hole has NULL link
  xlists.flist = xlists.hole;

  // set up the initial memory hole that represents our available heap
  h = xlists.flist;
  if (h == NULL) {
    return;
  }

  xlists.flist = h->link;

  h->addr = paddr((void *)KHEAP_START);
  h->size = KHEAP_INITIAL_SIZE;
  h->top = h->addr + h->size;
  h->link = NULL;

  // set this as our only table
  xlists.table = h;

  serial_debug("initial heap memory: paddr=%x vaddr=%x size=%x top=%x", h->addr,
               KHEAP_START, h->size, h->top);
  kheap = (void *)1;
}

void *xspanalloc(uint16_t size, int align, uint16_t span) {
  uintptr_t a, v, t;

  a = (uintptr_t)xalloc(size + align + span);
  if (a == 0)
    serial_debug("xspanalloc: %xd %d %xx", size, align, span);

  if (span > 2) {
    v = (a + span) & ~((uintptr_t)span - 1);
    t = v - a;
    if (t > 0)
      xhole(paddr((void *)a), t);
    t = a + span - v;
    if (t > 0)
      xhole(paddr((void *)(v + size + align)), t);
  } else
    v = a;

  if (align > 1)
    v = (v + align) & ~((uintptr_t)align - 1);

  return (void *)v;
}

void *xallocz(uint16_t size, int zero) {
  block_header_t *p;
  hole_t *h, **l;

  /* add room for magix & size overhead, round up to nearest vlong */
  size += BY2V + offsetof(block_header_t, data[0]);
  size &= ~(BY2V - 1);

  // ilock(&xlists);
  l = &xlists.table;
  for (h = *l; h; h = h->link) {
    if (h->size >= size) {
      p = (block_header_t *)kaddr(h->addr);
      h->addr += size;
      h->size -= size;
      if (h->size == 0) {
        *l = h->link;
        h->link = xlists.flist;
        xlists.flist = h;
      }
      // iunlock(&xlists);
      if (zero)
        memset(p, 0, size);
      p->magix = magichole;
      p->size = size;
      xsummary();
      // serial_debug("returning %x", p->data);
      return p->data;
    }
    l = &h->link;
  }
  // iunlock(&xlists);
  assert(0 && "out of memory");
  return nil;
}

void *xalloc(uint16_t size) { return xallocz(size, 1); }
/*
    157    block_header_t *x;
    158
    159    x = (block_header_t *)((uintptr_t)p - offsetof(block_header_t, data[0]));
    160    if (x->magix != magichole) {
    161      xsummary();
                          // p=0xc0101350  →  [...]  →  0xbaadf00d, x=0xc010133c  →  [...]  →  0xf000ff53
●→  162      serial_debug("corrupted magic(%x) %x != %x", p, magichole, x->magix);
    163      // x is 0 ??????
    164      hexdump((const char *)p - offsetof(block_header_t, data[0]) - 16, 64, 8);
    165    }
    166    xhole(paddr(x), x->size);
    167    serial_debug("freed %x", p);
─────────────────────────────────────────────────────────────────────────────────────────────────────────────── threads ────
[#0] Id 1, stopped 0x13bd9 in xfree (), reason: BREAKPOINT
───────────────────────────────────────────────────────────────────────────────────────────────────────────────── trace ────
[#0] 0x13bd9 → xfree(p=0xc0101000)
[#1] 0x141a7 → kfree(addr=0xc0101000)
[#2] 0x1005f → sched_test_dfn(df_data=0xc0101000)
[#3] 0x12f7e → task_wrapper(f=0x0)
[#4] 0x0 → push ebx
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
(remote) gef➤  p/x x
$1 = 0xc0100ff8
(remote) gef➤  p/x *x
$2 = {
  size = 0x0,
  magix = 0x0,
  data = 0xc0101000
}
(remote) gef➤


yet it complains
libc/heap.c:162 corrupted magic(0xc0101000) 0x484f4c45 != 0x084f4c45
what the hell?
*/
void xfree(void *p) {
  cli();
  block_header_t *x;

  x = (block_header_t *)((uintptr_t)p - offsetof(block_header_t, data[0]));
  if (x->magix != magichole) {
    xsummary();
    serial_debug("corrupted magic(%x) %x != %x", p, magichole, x->magix);
    // x is 0 ??????
    hexdump((const char *)p - offsetof(block_header_t, data[0]) - 16, 64, 8);
  }
  xhole(paddr(x), x->size);
  serial_debug("freed %x", p);
  sti();
}

int xmerge(void *vp, void *vq) {
  block_header_t *p, *q;

  p = (block_header_t *)(((uintptr_t)vp - offsetof(block_header_t, data[0])));
  q = (block_header_t *)(((uintptr_t)vq - offsetof(block_header_t, data[0])));
  if (p->magix != magichole || q->magix != magichole) {
    int i;
    uint16_t *wd;
    void *badp;

    xsummary();
    badp = (p->magix != magichole ? p : q);
    wd = (uint16_t *)badp - 12;
    for (i = 24; i-- > 0;) {
      serial_debug("%x: %xx", wd, *wd);
      if (wd == badp)
        serial_debug(" <-");
      serial_debug("");
      wd++;
    }
    serial_debug("xmerge(%x, %x) bad magic %xx, %xx", vp, vq, p->magix,
                 q->magix);
  }
  if ((uint8_t *)p + p->size == (uint8_t *)q) {
    p->size += q->size;
    return 1;
  }
  return 0;
}

void xhole(uintptr_t addr, uintptr_t size) {
  hole_t *h, *c, **l;
  uintptr_t top;

  if (size == 0)
    return;

  top = addr + size;
  // ilock(&xlists);
  l = &xlists.table;
  for (h = *l; h; h = h->link) {
    if (h->top == addr) {
      h->size += size;
      h->top = h->addr + h->size;
      c = h->link;
      if (c && h->top == c->addr) {
        h->top += c->size;
        h->size += c->size;
        h->link = c->link;
        c->link = xlists.flist;
        xlists.flist = c;
      }
      // iunlock(&xlists);
      return;
    }
    if (h->addr > addr)
      break;
    l = &h->link;
  }
  if (h && top == h->addr) {
    h->addr -= size;
    h->size += size;
    // iunlock(&xlists);
    return;
  }

  if (xlists.flist == nil) {
    // iunlock(&xlists);
    serial_debug("xfree: no free holes, leaked %dd bytes", size);
    return;
  }

  h = xlists.flist;
  xlists.flist = h->link;
  h->addr = addr;
  h->top = top;
  h->size = size;
  h->link = *l;
  *l = h;
  // iunlock(&xlists);
}

void xsummary(void) {
  int i;
  hole_t *h;
  uintptr_t s;

  serial_printff("xsummary:");
  i = 0;
  for (h = xlists.flist; h; h = h->link)
    i++;
  serial_printff(" %d holes free", i);

  s = 0;
  for (h = xlists.table; h; h = h->link) {
    serial_printff(" %x %x %dd", h->addr, h->top, h->size);
    s += h->size;
  }
  serial_printff(" %d bytes free", s);
}

int placement_address = PLACEMENT_ADDRESS;

void *kmalloc_int(uint32_t size, int align, uint32_t *phys) {
  serial_debug("somebody wants %d bytes aligned: %d w/ phys %x", size, align,
               phys);
  uint32_t addr = 0;
  if (kheap == NULL) {
    // early allocations before heap is initialized
    // use placement allocator instead
    addr = placement_address;

    if (align && (addr & 0xFFF)) {
      // align to page boundary (4KB)
      addr = (addr + PAGE_SIZE) & ~0xFFF;
    }

    if (phys) {
      *phys = addr; // physical address = virtual before paging
    }

    placement_address = addr + size;
  } else {
    if (align) {
      addr = (uint32_t)xspanalloc(size, PAGE_SIZE, 0);
    } else {
      addr = (uint32_t)xalloc(size);
    }

    if (phys && addr) {
      *phys = virt2phys((void *)addr);
    }
  }
  serial_debug("giving them the address %x", addr);
  return (void *)addr;
}

void kfree(void *addr) {
  if (addr == NULL) {
    serial_debug("nothing to free");
    return;
  } else if (kheap == NULL) {
    serial_debug("kheap is not active");
    return;
  }
  xfree(addr);
}

void *kmalloc_a(uint32_t size) { return kmalloc_int(size, 1, 0); }

void *kmalloc_p(uint32_t size, uint32_t *phys) {
  return kmalloc_int(size, 0, phys);
}

void *kmalloc_ap(uint32_t size, uint32_t *phys) {
  return kmalloc_int(size, 1, phys);
}

void *kmalloc(uint32_t size) { return kmalloc_int(size, 0, 0); }

void kheap_watchdog(void *data) {
  (void)data;
  serial_debug("heap watchdog started");

  while (1) {

    hole_t *h;
    int free_chunks = 0;
    uintptr_t free_memory = 0;
    bool free_list_corrupted = false;

    for (h = xlists.table; h != NULL; h = h->link) {
      free_chunks++;
      free_memory += h->size;

      if (h->addr + h->size != h->top) {
        serial_printff("FREE LIST CORRUPTION: Hole at %x has inconsistent "
                       "bounds (addr=%x, size=%d, top=%x)",
                       (uintptr_t)h, h->addr, h->size, h->top);
        free_list_corrupted = true;
      }

      if (h->link != NULL &&
          ((uintptr_t)h->link < KHEAP_START ||
           (uintptr_t)h->link >= KHEAP_START + KHEAP_INITIAL_SIZE)) {
        serial_printff("FREE LIST CORRUPTION: Hole at %x has invalid link %x",
                       (uintptr_t)h, (uintptr_t)h->link);
        free_list_corrupted = true;
      }
    }

// create a map of free regions to avoid checking them for allocated blocks
#define MAX_FREE_REGIONS 32
    struct {
      uintptr_t start;
      uintptr_t end;
    } free_regions[MAX_FREE_REGIONS];
    int free_region_count = 0;

    for (h = xlists.table; h != NULL && free_region_count < MAX_FREE_REGIONS;
         h = h->link) {
      free_regions[free_region_count].start = KHEAP_START + h->addr;
      free_regions[free_region_count].end = KHEAP_START + h->top;
      free_region_count++;
    }

    uintptr_t current_addr = KHEAP_START;
    int corrupted_blocks = 0;
    int valid_blocks = 0;
    uintptr_t allocated_memory = 0;

    while (current_addr < KHEAP_START + KHEAP_INITIAL_SIZE) {
      // check if current address is in a free region
      bool in_free_region = false;
      for (int i = 0; i < free_region_count; i++) {
        if (current_addr >= free_regions[i].start &&
            current_addr < free_regions[i].end) {
          // skip to the end of this free region
          current_addr = free_regions[i].end;
          in_free_region = true;
          break;
        }
      }

      if (in_free_region) {
        continue;
      }

      block_header_t *block = (block_header_t *)current_addr;

      // Basic validation for what might be a block header
      if (block->size >= sizeof(block_header_t) &&
          block->size < 65535 && // Max reasonable block size for uint16_t
          (current_addr + block->size) <= (KHEAP_START + KHEAP_INITIAL_SIZE)) {

        if (block->magix == magichole) {
          // this looks like a valid block
          valid_blocks++;
          allocated_memory += block->size;

          current_addr += block->size;
        } else {
          if ((block->size & 0x3) == 0 && // size should be aligned to 4 bytes
              block->size > 0) {

            serial_printff("POSSIBLE CORRUPTED BLOCK at %x: size=%d, magix=%x "
                           "(expected %x)",
                           current_addr, block->size, block->magix, magichole);

            hexdump((const char *)current_addr - 16, 64, 16);
            corrupted_blocks++;

            current_addr += block->size;
          } else {
            current_addr += 4;
          }
        }
      } else {
        current_addr += 4;
      }
    }

    serial_printff("heap watchdog stats: %d valid blocks (%d bytes), %d "
                   "corrupted blocks, %d free holes (%d bytes)",
                   valid_blocks, allocated_memory, corrupted_blocks,
                   free_chunks, free_memory);

    if (free_list_corrupted) {
      serial_printff("WARNING: Free list integrity check failed!");
    }

    serial_printff("heap summary from xlists:");
    xsummary();

    for (volatile int i = 0; i < 5000000; i++) {
      asm volatile("nop");
    }
  }
}